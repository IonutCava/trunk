#ifndef _VELOCITY_CALC_FRAG_
#define _VELOCITY_CALC_FRAG_

//ref: https://developer.nvidia.com/gpugems/gpugems3/part-iv-image-effects/chapter-27-motion-blur-post-processing-effect
vec2 velocityCalc() {
#   if defined(HAS_VELOCITY)
    const vec4 posWVP = dvd_ProjectionMatrix * VAR._vertexWV;
    vec2 a = (posWVP.xy / posWVP.w) * 0.5f + 0.5f;
    vec2 b = (VAR._prevVertexWVP.xy / VAR._prevVertexWVP.w) * 0.5f + 0.5f;
    return a - b;

#else
    return vec2(0.0f);
#endif
}

#endif //_VELOCITY_CALC_FRAG_
