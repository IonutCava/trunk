#ifndef _VSM_FRAG_
#define _VSM_FRAG_

#include "utility.frag"

vec2 computeMoments(const float Depth) {
    vec2 Moments;  
    // First moment is the depth itself.
    Moments.x = Depth;
    // Compute partial derivatives of depth. 
    const float dx = dFdx(Depth); 
    const float dy = dFdy(Depth);
    // Compute second moment over the pixel extents.
    Moments.y = Depth*Depth + 0.25f*(dx*dx + dy*dy);  
    return Moments;
}

vec2 computeMoments() {
    if (SHADOW_PASS_TYPE == SHADOW_PASS_ORTHO) {
        return computeMoments(gl_FragCoord.z);
    } else /*if (SHADOW_PASS_TYPE == SHADOW_PASS_NORMAL || SHADOW_PASS_TYPE == SHADOW_PASS_CUBE) */ {
        const float distanceToLight = length(VAR._vertexW.xyz - dvd_cameraPosition.xyz);
        return computeMoments(distanceToLight / dvd_zPlanes.y);
    }
}

#endif //_VSM_FRAG_