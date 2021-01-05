#ifndef _VELOCITY_CALC_FRAG_
#define _VELOCITY_CALC_FRAG_

//ref: https://developer.nvidia.com/gpugems/gpugems3/part-iv-image-effects/chapter-27-motion-blur-post-processing-effect
#if defined(HAS_VELOCITY)
vec2 velocityCalc() {
    const vec4 vertexWVP = dvd_ProjectionMatrix * VAR._vertexWV;

    const vec2 a = homogenize(vertexWVP).xy * 0.5f + 0.5f;
    const vec2 b = VAR._prevVertexWVP_XYW.xy / VAR._prevVertexWVP_XYW.z * 0.5f + 0.5f;
    vec2 vel = (a - b);
    vel.x = pow(vel.x, 3.0f);
    vel.y = pow(vel.y, 3.0f);
    return vel;
}
#else //HAS_VELOCITY
#define velocityCalc() vec2(0.0f)
#endif //HAS_VELOCITY

#endif //_VELOCITY_CALC_FRAG_
