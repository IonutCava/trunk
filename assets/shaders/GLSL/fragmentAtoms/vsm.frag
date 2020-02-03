#ifndef _VSM_FRAG_
#define _VSM_FRAG_

float getShadowDepth() {
    const float depth = gl_FragCoord.z;
#if 0
    return exp(DEPTH_EXP_WARP * depth);
#endif
    return depth;
}

vec2 computeMoments() {
    const float depth = getShadowDepth();
    // Adjusting moments (this is sort of bias per pixel) using partial derivative
    // Compute second moment over the pixel extents.  
    return vec2(depth, pow(depth, 2.0f) + 0.25f * (pow(dFdx(depth), 2.0f) + pow(dFdy(depth), 2.0f)));
}


#endif //_VSM_FRAG_