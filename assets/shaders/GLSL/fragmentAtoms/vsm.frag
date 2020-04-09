#ifndef _VSM_FRAG_
#define _VSM_FRAG_

vec2 computeMoments(const float depth) {
    // Adjusting moments (this is sort of bias per pixel) using partial derivative
    // Compute second moment over the pixel extents.  
    return vec2(depth, pow(depth, 2.0f) + 0.25f * (pow(dFdx(depth), 2.0f) + pow(dFdy(depth), 2.0f)));
}

vec2 computeMoments() {
    return computeMoments(gl_FragCoord.z);
}

#endif //_VSM_FRAG_