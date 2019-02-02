#ifndef _SHADOW_DIRECTIONAL_FRAG_
#define _SHADOW_DIRECTIONAL_FRAG_

#define LT(n) n, n, n, n, n, n, n, n, n, n, n, n, n, n, n, n

//Chebyshev Upper Bound
float VSM(vec2 moments, float compare) {
    float p = smoothstep(compare - 0.02, compare, moments.x);
    float variance = max(moments.y - moments.x * moments.x, -(dvd_shadowingSettings.y));
    float d = compare - moments.x;
    float p_max = linstep(dvd_shadowingSettings.x, 1.0, variance / (variance + d * d));
    return clamp(max(p, p_max), 0.0, 1.0);
}

float applyShadowDirectional(in uint idx, in uvec4 details, in float fragDepth) {
    // find the appropriate depth map to look up in based on the depth of this fragment
    g_shadowTempInt = 0;
    // Figure out which cascade to sample from
    for (g_shadowTempInt = 0; g_shadowTempInt < int(MAX_CSM_SPLITS_PER_LIGHT); g_shadowTempInt++) {
        if (fragDepth > dvd_shadowLightPosition[g_shadowTempInt + (idx * 6)].w) {
            break;
        }
    }

    // GLOBAL
    const int SplitPowLookup[] = { 0, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3,
                                   LT(4), LT(5), LT(5), LT(6), LT(6), LT(6), LT(6),
                                   LT(7), LT(7), LT(7), LT(7), LT(7), LT(7), LT(7), LT(7)};
    // Ensure that every fragment in the quad choses the same split so that derivatives
    // will be meaningful for proper texture filtering and LOD selection.
    int SplitPow = 1 << g_shadowTempInt;
    int SplitX = int(abs(dFdx(SplitPow)));
    int SplitY = int(abs(dFdy(SplitPow)));
    int SplitXY = int(abs(dFdx(SplitY)));
    int SplitMax = max(SplitXY, max(SplitX, SplitY));
    g_shadowTempInt = SplitMax > 0 ? SplitPowLookup[SplitMax - 1] : g_shadowTempInt;

    vec4 sc = dvd_shadowLightVP[g_shadowTempInt + (idx * 6)] * VAR._vertexW;
    vec3 shadowCoord = (sc.xyz / sc.w) * 0.5 + 0.5;

    bool inFrustum = all(bvec4(
        shadowCoord.x >= 0.0,
        shadowCoord.x <= 1.0,
        shadowCoord.y >= 0.0,
        shadowCoord.y <= 1.0));

    if (inFrustum && shadowCoord.z <= 1.0)
    {
        float layer = float(g_shadowTempInt + details.y);

        vec2 moments = texture(texDepthMapFromLightArray, vec3(shadowCoord.xy, layer)).rg;
       
        //float shadowBias = DEPTH_EXP_WARP * exp(DEPTH_EXP_WARP * dvd_shadowingSettings.y);
        //float shadowWarpedz1 = exp(shadowCoord.z * DEPTH_EXP_WARP);
        //return mix(VSM(moments, shadowWarpedz1), 
        //             1.0, 
        //             clamp(((gl_FragCoord.z + dvd_shadowingSettings.z) - dvd_shadowingSettings.w) / dvd_shadowingSettings.z, 0.0, 1.0));

        return max(VSM(moments, sc.z / sc.w), 0.02);
    }
    
    return 1.0;
}
#endif //_SHADOW_DIRECTIONAL_FRAG_