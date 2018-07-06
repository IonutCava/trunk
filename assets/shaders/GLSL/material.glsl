-- Vertex

#include "vbInputData.vert"
#include "lightingDefaults.vert"

#if defined(ADD_FOLIAGE)
#include "foliage.vert"
#endif

#ifndef USE_COLOURDE_WOIT
//#define USE_COLOURED_WOIT
#endif //USE_COLOURED_WOIT

void main(void){

    computeData();

#if defined(ADD_FOLIAGE) && defined(IS_TREE)
    computeFoliageMovementTree(dvd_Vertex);
#endif
    
    computeLightVectors();

    //Compute the final vert position
    gl_Position = dvd_ViewProjectionMatrix * VAR._vertexW;

}

-- Fragment

#if defined(USE_OPACITY_DIFFUSE) || defined(USE_OPACITY_MAP) || defined(USE_OPACITY_DIFFUSE_MAP)
#   define HAS_TRANSPARENCY
#endif

#if !defined(HAS_TRANSPARENCY)
layout(early_fragment_tests) in;
#endif

#include "BRDF.frag"

#if defined(COMPUTE_TBN)
    #include "bumpMapping.frag"
#endif

#include "velocityCalc.frag"

#if defined(OIT_PASS)
layout(location = 0) out vec4  _accum;
layout(location = 1) out float _revealage;
layout(location = 2) out vec4  _modulate;
#else
layout(location = 0) out vec4 _colourOut;
layout(location = 1) out vec2 _normalOut;
layout(location = 2) out vec2 _velocityOut;
#endif

//subroutine vec4 MappingRoutineType();

//layout(location = 0) subroutine uniform MappingRoutineType MappingRoutine;

//subroutine(MappingRoutineType)
vec4 mappingFlat(){
    return getPixelColour(VAR._texCoord, VAR._normalWV);
}

#if defined(COMPUTE_TBN)
//subroutine(MappingRoutineType)
vec4 mappingNormal(){
    vec3 bump = getBump(VAR._texCoord);
    return mix(getPixelColour(VAR._texCoord, getTBNMatrix() * bump),
               mappingFlat(),
               vec4(dvd_lodLevel > 1));
}

//subroutine(MappingRoutineType)
vec4 mappingRelief(){
    return mix(ReliefMapping(VAR._texCoord),
               mappingFlat(),
               vec4(dvd_lodLevel > 1));
}

//subroutine(MappingRoutineType)
vec4 mappingParallax(){
    return mix(ParallaxMapping(VAR._texCoord),
               mappingFlat(),
               vec4(dvd_lodLevel > 1));
}
#endif

vec4 getFinalPixelColour() {
//return MappingRoutine();
#if defined(COMPUTE_TBN)
    bumpInit();
#    if defined(USE_PARALLAX_MAPPING)
    return mappingParallax();
#    elif defined(USE_RELIEF_MAPPING)
    return mappingRelief();
#    else
    return mappingNormal();
#    endif
#else
   return mappingFlat();
#endif
}


#if defined(OIT_PASS)
#if USE_COLOURED_WOIT
// Shameless copy-paste from http://casual-effects.blogspot.co.uk/2015/03/colored-blended-order-independent.html
void writePixel(vec4 premultipliedReflect, vec3 transmit, float csZ) {
    // NEW: Perform this operation before modifying the coverage to account for transmission.
    _modulate.rgb = premultipliedReflect.a * (vec3(1.0) - transmit);

    // Modulate the net coverage for composition by the transmission. This does not affect the color channels of the
    //transparent surface because the caller's BSDF model should have already taken into account if transmission modulates
    //reflection. See

    //McGuire and Enderton, Colored Stochastic Shadow Maps, ACM I3D, February 2011
    //http://graphics.cs.williams.edu/papers/CSSM/

    //for a full explanation and derivation.
    premultipliedReflect.a *= 1.0 - (transmit.r + transmit.g + transmit.b) * (1.0 / 3.0);

    // Intermediate terms to be cubed
    float tmp = (premultipliedReflect.a * 8.0 + 0.01) *  (-gl_FragCoord.z * 0.95 + 1.0);

    // If a lot of the scene is close to the far plane, then gl_FragCoord.z does not
    //provide enough discrimination. Add this term to compensate:

    tmp /= sqrt(abs(csZ)); 

    float w = clamp(tmp * tmp * tmp * 1e3, 1e-2, 3e2);
    _accum = premultipliedReflect * w;
    _revealage = premultipliedReflect.a;
}
#else
void writePixel(vec4 premultipliedReflect, float csZ) {
    vec3 transmit = vec3(0.2, 0.1, 0.0);
    //premultipliedReflect.a *= 1.0 - clamp((transmit.r + transmit.g + transmit.b) * (1.0 / 3.0), 0, 1);

    /* You may need to adjust the w function if you have a very large or very small view volume; see the paper and
    presentation slides at http://jcgt.org/published/0002/02/09/ */
    // Intermediate terms to be cubed
    float a = min(1.0, premultipliedReflect.a) * 8.0 + 0.01;

    float b = -gl_FragCoord.z * 0.95 + 1.0;

    /* If your scene has a lot of content very close to the far plane,
    then include this line (one rsqrt instruction):*/
    //b /= sqrt(1e4 * abs(csZ));

    float w = clamp(a * a * a * 1e8 * b * b * b, 1e-2, 3e2);
    _accum = premultipliedReflect * w;

    _revealage = premultipliedReflect.a;
}
#endif

void nvidiaSample(in vec4 color, in float linearDepth) {
    // Tuned to work well with FP16 accumulation buffers and 0.001 < linearDepth < 2.5
    // See Equation (9) from http://jcgt.org/published/0002/02/09/
    float weight = clamp(0.03 / (1e-5 + pow(linearDepth, 4.0)), 1e-2, 3e3);
    _accum = vec4(color.rgb * color.a, color.a) * weight;
    _revealage = color.a;
}

#endif

void main (void){
    vec4 colourOut = vec4(0.0);

#if defined(_DEBUG)
    if (dvd_NormalsOnly) {
        colourOut.rgb = getProcessedNormal();
    } else {
        colourOut = getFinalPixelColour();
    }
#else
    colourOut = getFinalPixelColour();
#endif

    vec2 screenPositionNormalised = getScreenPositionNormalised();
#if defined(OIT_PASS)
    float linearDepth = ToLinearDepth(getDepthValue(screenPositionNormalised));

#if defined(USE_COLOURED_WOIT)
    //writePixel(colourOut, colourOut.rgb - vec3(0.2), linearDepth);
#else //USE_COLOURED_WOIT
    //writePixel(colourOut, linearDepth);
#endif //USE_COLOURED_WOIT

    nvidiaSample(colourOut, linearDepth);
#else //OIT_PASS

    _colourOut = colourOut;
    _normalOut = packNormal(getProcessedNormal());
    _velocityOut = velocityCalc(dvd_InvProjectionMatrix, screenPositionNormalised);

#endif //OIT_PASS
}
