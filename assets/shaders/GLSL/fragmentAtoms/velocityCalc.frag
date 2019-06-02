#ifndef _VELOCITY_CALC_FRAG_
#define _VELOCITY_CALC_FRAG_

layout(binding = TEXTURE_DEPTH_MAP_PREV) uniform sampler2D texDepthMapPrev;

vec2 velocityCalc(float crtDepth, mat4 invProj, vec2 screenNormalisedPos) {
    float prevDepth = textureLod(texDepthMapPrev, screenNormalisedPos, 0).r;

    vec4 crtPos  = positionFromDepth(crtDepth, invProj, screenNormalisedPos);
    vec4 prevPos = positionFromDepth(prevDepth, invProj, screenNormalisedPos);

    vec4 velocity = (crtPos - prevPos) * 0.5f;
    return velocity.xy;
}

#endif //_VELOCITY_CALC_FRAG_
