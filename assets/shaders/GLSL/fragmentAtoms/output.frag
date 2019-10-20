#ifndef _OUTPUT_FRAG_
#define _OUTPUT_FRAG_

#if !defined(OIT_PASS)
layout(location = TARGET_ALBEDO) out vec4 _colourOut;
#else //OIT_PASS
#define USE_NVIDIA_SAMPLE
#define uDepthScale 0.5f

layout(location = TARGET_ACCUMULATION) out vec4  _accum;
layout(location = TARGET_REVEALAGE) out float _revealage;

#if defined(USE_COLOURED_WOIT)
layout(location = TARGET_MODULATE) out vec4  _modulate;
#endif

// Shameless copy-paste from http://casual-effects.blogspot.co.uk/2015/03/colored-blended-order-independent.html
void writePixel(vec4 premultipliedReflect, vec3 transmit, float csZ) {
#if defined(USE_NVIDIA_SAMPLE)
    csZ *= uDepthScale;
    // Tuned to work well with FP16 accumulation buffers and 0.001 < linearDepth < 2.5
    // See Equation (9) from http://jcgt.org/published/0002/02/09/

    //float weight = pow(color.a, 1.0) * clamp(0.03f / (1e-5f + pow(linearDepth, 4.0f)), 1e-2f, 3e3f);
    float w = clamp(0.03f / (1e-5f + pow(csZ, 4.0f)), 1e-2f, 3e3f);
#else //USE_NVIDIA_SAMPLE

#if defined(USE_COLOURED_WOIT)
    // NEW: Perform this operation before modifying the coverage to account for transmission.
    _modulate.rgb = premultipliedReflect.a * (vec3(1.0) - transmit);
#endif

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
#endif //USE_NVIDIA_SAMPLE

    _accum = premultipliedReflect * w;
    _revealage = premultipliedReflect.a;
}
#endif //OIT_PASS

void writeOutput(vec4 colour) {
#if defined(OIT_PASS)
    vec3 transmit = vec3(0.2, 0.1, 0.0);
#   if defined(USE_NVIDIA_SAMPLE)
        float linearDepth = abs(1.0 / gl_FragCoord.w);
#   else
        float linearDepth = ToLinearDepth(getDepthValue(dvd_screenPositionNormalised));
#   endif

    colour.rgb *= colour.a;
    writePixel(colour, colour.rgb - transmit, linearDepth);

#else //OIT_PASS
    _colourOut = colour;
    // write depth value to alpha for refraction?
#   if defined(WRITE_DEPTH_TO_ALPHA)
        //ToDo: _colourOut.a = depth;
#   endif
#endif //OIT_PASS
}

#endif //_OUTPUT_FRAG_