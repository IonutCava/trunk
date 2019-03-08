#ifndef _VSM_FRAG_
#define _VSM_FRAG_

/*float getShadowDepth() {
    float depth = gl_FragCoord.z;
    depth = exp(DEPTH_EXP_WARP * depth);
    return depth;
}*/

vec2 computeMoments() {
    float depth = gl_FragCoord.z;

    // Adjusting moments (this is sort of bias per pixel) using partial derivative
    float dx = dFdx(depth);
    float dy = dFdy(depth);
    // Compute second moment over the pixel extents.  
    return vec2(depth, pow(depth, 2.0) + 0.25 * (dx * dx + dy * dy));
}


#endif //_VSM_FRAG_