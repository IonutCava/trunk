#ifndef _SHADOW_DIRECTIONAL_FRAG_
#define _SHADOW_DIRECTIONAL_FRAG_

#define LT(n) n, n, n, n, n, n, n, n, n, n, n, n, n, n, n, n

//Chebyshev Upper Bound
float VSM(vec2 moments, float fragDepth) {
    float lit = 0.0f;

    float E_x2 = moments.y;
    float Ex_2 = moments.x * moments.x;
    float variance = max(E_x2 - Ex_2, -(dvd_shadowingSettings.y));
    float mD = (moments.x - fragDepth);
    float mD_2 = mD * mD;
    float p = linstep(dvd_shadowingSettings.x, 1.0, variance / (variance + mD_2));
    lit = max(p, fragDepth <= moments.x ? 1.0 : 0.0);

    return clamp(lit, 0.0, 1.0);
}

// find the appropriate depth map to look up in based on the depth of this fragment
int getCSMSlice(in uint idx) {

    int shadowTempInt = 0;
    const float fragDepth = VAR._vertexWV.z;
    // Figure out which cascade to sample from

    float dist = 0.0f;
    for (; shadowTempInt < MAX_CSM_SPLITS_PER_LIGHT; shadowTempInt++) {
        dist = dvd_shadowLightPosition[shadowTempInt + (idx * 6)].w;
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
        LT(5), LT(5)
#       endif //MAX_CSM_SPLITS_PER_LIGHT
    };

    // Ensure that every fragment in the quad choses the same split so that derivatives
    // will be meaningful for proper texture filtering and LOD selection.
    const int SplitPow = 1 << shadowTempInt;
    const int SplitX = int(abs(dFdx(SplitPow)));
    const int SplitY = int(abs(dFdy(SplitPow)));
    const int SplitXY = int(abs(dFdx(SplitY)));
    const int SplitMax = max(SplitXY, max(SplitX, SplitY));
    
    return SplitMax > 0 ? SplitPowLookup[SplitMax - 1] : shadowTempInt;
}

float applyShadowDirectional(in uint idx, in uvec4 details) {
    const int shadowTempInt = getCSMSlice(idx);

    const vec4 sc = dvd_shadowLightVP[shadowTempInt + (idx * 6)] * VAR._vertexW;
    const vec3 shadowCoord = sc.xyz / sc.w;

    bool inFrustum = all(bvec4(
        shadowCoord.x >= 0.0,
        shadowCoord.x <= 1.0,
        shadowCoord.y >= 0.0,
        shadowCoord.y <= 1.0));

    if (inFrustum && shadowCoord.z <= 1.0)
    {
        float layer = float(shadowTempInt + details.y);

        vec2 moments = texture(texDepthMapFromLightArray, vec3(shadowCoord.xy, layer)).rg;
       
        //float shadowBias = DEPTH_EXP_WARP * exp(DEPTH_EXP_WARP * dvd_shadowingSettings.y);
        //float shadowWarpedz1 = exp(shadowCoord.z * DEPTH_EXP_WARP);
        //return mix(VSM(moments, shadowWarpedz1), 
        //             1.0, 
        //             clamp(((gl_FragCoord.z + dvd_shadowingSettings.z) - dvd_shadowingSettings.w) / dvd_shadowingSettings.z, 0.0, 1.0));

        //float bias = max(angleBias * (1.0 - dot(normal, lightDirection)), 0.0008);
        float bias = 0.0f;
        return max(VSM(moments, shadowCoord.z - bias), 0.2);
    }
    
    return 1.0;
}
#endif //_SHADOW_DIRECTIONAL_FRAG_