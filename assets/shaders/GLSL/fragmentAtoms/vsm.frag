#ifndef _VSM_FRAG_
#define _VSM_FRAG_

/*float getShadowDepth() {
    float depth = gl_FragCoord.z;
    depth = exp(DEPTH_EXP_WARP * depth);
    return depth;
}*/

vec2 computeMoments() {
    const float depth = gl_FragCoord.z;
    // Adjusting moments (this is sort of bias per pixel) using partial derivative
    // Compute second moment over the pixel extents.  
    return vec2(depth, pow(depth, 2.0f) + 0.25f * (pow(dFdx(depth), 2.0f) + pow(dFdy(depth), 2.0f)));
}


#endif //_VSM_FRAG_