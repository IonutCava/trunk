#ifndef _VSM_FRAG_
#define _VSM_FRAG_

#include "utility.frag"

vec2 computeMoments(const float depth) {
    // Adjusting moments (this is sort of bias per pixel) using partial derivative
    // Compute second moment over the pixel extents.  
    return vec2(depth, pow(depth, 2.0f) + 0.25f * (pow(dFdx(depth), 2.0f) + pow(dFdy(depth), 2.0f)));
}

vec2 computeMoments() {
    if (SHADOW_PASS_TYPE == SHADOW_PASS_ORTHO) {
        return computeMoments(gl_FragCoord.z);
    } else if (SHADOW_PASS_TYPE == SHADOW_PASS_NORMAL) {
        const float depth = VAR._vertexWVP.z / VAR._vertexWVP.w;
        return computeMoments(depth * 0.5f + 0.5f);
    } else /*(SHADOW_PASS_TYPE == SHADOW_PASS_CUBE)*/ {
        const float depth = length(VAR._vertexW.xyz - dvd_cameraPosition.xyz) / dvd_zPlanes.y;
        return computeMoments(depth);
    }
}

#endif //_VSM_FRAG_