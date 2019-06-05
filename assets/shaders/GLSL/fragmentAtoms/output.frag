#ifndef _OUTPUT_FRAG_
#define _OUTPUT_FRAG_

#if defined(OIT_PASS)
#define uDepthScale 0.5f

layout(location = TARGET_ACCUMULATION) out vec4  _accum;
layout(location = TARGET_REVEALAGE) out float _revealage;
layout(location = TARGET_MODULATE) out vec4  _modulate;
#else
layout(location = TARGET_ALBEDO) out vec4 _colourOut;
#endif


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

float nvidiaSample(in vec4 color, in float linearDepth) {
    // Tuned to work well with FP16 accumulation buffers and 0.001 < linearDepth < 2.5
    // See Equation (9) from http://jcgt.org/published/0002/02/09/
    linearDepth *= uDepthScale;

    //float weight = pow(color.a, 1.0) * clamp(0.03f / (1e-5f + pow(linearDepth, 4.0f)), 1e-2f, 3e3f);
    float weight = clamp(0.03f / (1e-5f + pow(linearDepth, 4.0f)), 1e-2f, 3e3f);
    _accum = vec4(color.rgb * color.a, color.a) * weight;
    _revealage = color.a;

    return weight;
}
#endif

void writeOutput(vec4 colour) {
#if defined(OIT_PASS)
    float linearDepth = abs(1.0 / gl_FragCoord.w);
    //float linearDepth = ToLinearDepth(getDepthValue(dvd_screenPositionNormalised));

#if defined(USE_COLOURED_WOIT)
    //writePixel(colour, colour.rgb - vec3(0.2f), linearDepth);
#else //USE_COLOURED_WOIT
    //writePixel(colour, linearDepth);
#endif //USE_COLOURED_WOIT

    nvidiaSample(colour, linearDepth);
#else //OIT_PASS

    // write depth value to alpha for refraction?
    _colourOut = colour;
#if defined(WRITE_DEPTH_TO_ALPHA)
    //ToDo
    _colourOut.a = 0.5f;
#endif
#endif //OIT_PASS
}

void writeOutput(vec3 colour) {
    writeOutput(vec4(colour, 1.0f));
}

#endif //_OUTPUT_FRAG_