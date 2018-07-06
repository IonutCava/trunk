-- Vertex

#include "vbInputData.vert"
#include "lightingDefaults.vert"

#if defined(ADD_FOLIAGE)
#include "foliage.vert"
#endif

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

#if defined(USE_WB_OIT)
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


#if defined(USE_WB_OIT)
// Shameless copy-paste from http://casual-effects.blogspot.co.uk/2015/03/colored-blended-order-independent.html
void writePixel(vec4 premultipliedReflect, vec3 transmit, float csZ, inout vec4 accum, inout float revealage) {
    /* NEW: Perform this operation before modifying the coverage to account for transmission. */
    _modulate.rgb = premultipliedReflect.a * (vec3(1.0) - transmit);

    /* Modulate the net coverage for composition by the transmission. This does not affect the color channels of the
    transparent surface because the caller's BSDF model should have already taken into account if transmission modulates
    reflection. See

    McGuire and Enderton, Colored Stochastic Shadow Maps, ACM I3D, February 2011
    http://graphics.cs.williams.edu/papers/CSSM/

    for a full explanation and derivation.*/
    premultipliedReflect.a *= 1.0 - (transmit.r + transmit.g + transmit.b) * (1.0 / 3.0);

    // Intermediate terms to be cubed
    float tmp = (premultipliedReflect.a * 8.0 + 0.01) *  (-gl_FragCoord.z * 0.95 + 1.0);

    /* If a lot of the scene is close to the far plane, then gl_FragCoord.z does not
    provide enough discrimination. Add this term to compensate:

    tmp /= sqrt(abs(csZ)); */

    float w = clamp(tmp * tmp * tmp * 1e3, 1e-2, 3e2);
    _accum = premultipliedReflect * w;
    _revealage = premultipliedReflect.a;
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
#if defined(USE_WB_OIT)
    float linearDepth = ToLinearDepth(getDepthValue(screenPositionNormalised));
    writePixel(colourOut, vec3(1.0), linearDepth, _accum, _revealage);
#else
    _colourOut = colourOut;
    _normalOut = packNormal(getProcessedNormal());
    _velocityOut = velocityCalc(dvd_InvProjectionMatrix, screenPositionNormalised);
#endif
}
