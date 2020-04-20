#ifndef _SHADOW_UTILS_FRAG_
#define _SHADOW_UTILS_FRAG_

layout(binding = BUFFER_LIGHT_SHADOW, std140) uniform dvd_ShadowBlock
{
    // x = light type, y =  arrayOffset, z - bias
    vec4  dvd_shadowLightDetails[MAX_SHADOW_CASTING_LIGHTS];
    vec4  dvd_shadowLightPosition[MAX_SHADOW_CASTING_LIGHTS * 6];
    mat4  dvd_shadowLightVP[MAX_SHADOW_CASTING_LIGHTS * 6];
};

// find the appropriate depth map to look up in based on the depth of this fragment
int getCSMSlice(in int idx) {

    int Split = 0;
    const float fragDepth = (VAR._vertexWV.z / VAR._vertexWV.w);
    // Figure out which cascade to sample from

    for (; Split < MAX_CSM_SPLITS_PER_LIGHT; Split++) {
        const float dist = -dvd_shadowLightPosition[Split + (idx * 6)].w;
        if (fragDepth > dist) {
            break;
        }
    }

    // GLOBAL
    const int SplitPowLookup[] = {
        0,
        1, 1,
        2, 2, 2, 2,
        3
#       if MAX_CSM_SPLITS_PER_LIGHT > 4
        , 3, 3, 3, 3, 3, 3, 3,
        LT(4),
        LT(5), LT(5),
        LT(6), LT(6), LT(6), LT(6)
#       endif //MAX_CSM_SPLITS_PER_LIGHT
    };

    // Ensure that every fragment in the quad choses the same split so that derivatives
    // will be meaningful for proper texture filtering and LOD selection.
    const int SplitPow = 1 << Split;
    const int SplitX = int(abs(dFdx(SplitPow)));
    const int SplitY = int(abs(dFdy(SplitPow)));
    const int SplitXY = int(abs(dFdx(SplitY)));
    const int SplitMax = max(SplitXY, max(SplitX, SplitY));

    return SplitMax > 0 ? SplitPowLookup[SplitMax - 1] : Split;
}
#endif //_SHADOW_UTILS_FRAG_