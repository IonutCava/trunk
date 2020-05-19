#ifndef _VELOCITY_CALC_FRAG_
#define _VELOCITY_CALC_FRAG_

//ref: https://developer.nvidia.com/gpugems/gpugems3/part-iv-image-effects/chapter-27-motion-blur-post-processing-effect
vec2 velocityCalc() {
#   if defined(HAS_VELOCITY)
    const vec2 a = (VAR._vertexWVP.xy / VAR._vertexWVP.w) * 0.5f + 0.5f;
    const vec2 b = (VAR._prevVertexWVP.xy / VAR._prevVertexWVP.w) * 0.5f + 0.5f;
    vec2 vel = (a - b);
    vel.x = pow(vel.x, 3.0f);
    vel.y = pow(vel.y, 3.0f);
    return vel;

#else
    return vec2(0.0f);
#endif
}

#endif //_VELOCITY_CALC_FRAG_
