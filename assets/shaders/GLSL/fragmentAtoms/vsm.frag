#ifndef _VSM_FRAG_
#define _VSM_FRAG_

vec2 computeMoments() {
#if defined(ORTHO_PROJECTION)
#define Depth gl_FragCoord.z
#else //ORTHO_PROJECTION
const float Depth = length(VAR._vertexW.xyz - dvd_cameraPosition.xyz) / dvd_zPlanes.y;
#endif //ORTHO_PROJECTION

    // Compute partial derivatives of depth. 
    const float dx = dFdx(Depth);
    const float dy = dFdy(Depth);

    // First moment is the depth itself.
    // Compute second moment over the pixel extents.
    return vec2(Depth, Depth * Depth + 0.25f * (dx * dx + dy * dy));
}

#endif //_VSM_FRAG_
