#ifndef _OUTPUT_FRAG_
#define _OUTPUT_FRAG_

#if !defined(OIT_PASS)
layout(location = TARGET_ALBEDO) out vec4 _colourOut;
#else //OIT_PASS

layout(location = TARGET_ACCUMULATION) out vec4  _accum;
layout(location = TARGET_REVEALAGE) out float _revealage;

#if defined(USE_COLOURED_WOIT)
layout(location = TARGET_MODULATE) out vec4  _modulate;
#endif

// Shameless copy-paste from http://casual-effects.blogspot.co.uk/2015/03/colored-blended-order-independent.html
void writePixel(vec4 premultipliedReflect, vec3 transmit, float csZ) {
    float alpha = premultipliedReflect.a;
#if defined(USE_COLOURED_WOIT)
    // NEW: Perform this operation before modifying the coverage to account for transmission.
    _modulate.rgb = alpha * (vec3(1.0) - transmit);
#endif

    // Modulate the net coverage for composition by the transmission. This does not affect the color channels of the
    //transparent surface because the caller's BSDF model should have already taken into account if transmission modulates
    //reflection. See

    //McGuire and Enderton, Colored Stochastic Shadow Maps, ACM I3D, February 2011
    //http://graphics.cs.williams.edu/papers/CSSM/

    //for a full explanation and derivation.
    //alpha *= 1.0f - saturate((transmit.r + transmit.g + transmit.b) * 0.33f);

    // You may need to adjust the w function if you have a very large or very small view volume; see the paper and
    // presentation slides at http://jcgt.org/published/0002/02/09/ 
    // Intermediate terms to be cubed
    float a = min(1.0f, alpha) * 8.0f + 0.01f;
    float b = -gl_FragCoord.z * 0.95f + 1.0f;

    // If your scene has a lot of content very close to the far plane, then include this line (one rsqrt instruction):
    //b /= sqrt(1e4 * abs(csZ));

    float w = clamp(a * a * a * 1e3 * b * b * b, 1e-2, 3e2);

    _accum = premultipliedReflect * w;
    _revealage = alpha;
}
#endif //OIT_PASS

#if defined(OIT_PASS)
void writeScreenColour(in vec4 colour) {
    const vec3 transmit = vec3(0.0f);// texture(texTransmitance, dvd_screenPositionNormalised).rgb;
    const float linearDepth = ToLinearDepth(textureLod(texDepthMap, dvd_screenPositionNormalised, 0).r);

    writePixel(vec4(colour.rgb * colour.a, colour.a), transmit, linearDepth);
}
#else //OIT_PASS
// write depth value to alpha for refraction?
#define writeScreenColour(colour) _colourOut = colour
#endif //OIT_PASS

#endif //_OUTPUT_FRAG_
