#ifndef _SHADOW_DIRECTIONAL_FRAG_
#define _SHADOW_DIRECTIONAL_FRAG_

#define LT(n) n, n, n, n, n, n, n, n, n, n, n, n, n, n, n, n

// Reduces VSM light bleeding (remove the [0, amount] tail and linearly rescale (amount, 1])
float reduceLightBleeding(float pMax, float amount) {
    return linstep(amount, 1.0f, pMax);
}

float chebyshevUpperBound(vec2 moments, float distance, float minVariance) {
    if (distance <= moments.x + dvd_shadowingSettings.x) {
        return 1.0;
    }

    float variance = max(moments.y - (moments.x * moments.x), minVariance);

    float d = distance - moments.x;
    float p_max = variance / (variance + d*d);
    //return max(1.0 - p_max, 0.0);
    return p_max;
}

float VSM(vec2 moments, float compare) {
    float p = smoothstep(compare - 0.02, compare, moments.x);
    float variance = max(moments.y - moments.x*moments.x, -0.001);
    float d = compare - moments.x;
    float p_max = linstep(0.2, 1.0, variance / (variance + d * d));
    return clamp(max(p, p_max), 0.0, 1.0);
}

float applyShadowDirectional(in uint idx, in uvec4 details, in float fragDepth) {
    Shadow currentShadowSource = dvd_ShadowSource[idx];

    // find the appropriate depth map to look up in based on the depth of this fragment
    g_shadowTempInt = 0;
    // Figure out which cascade to sample from
    for (g_shadowTempInt = 0; g_shadowTempInt < int(MAX_CSM_SPLITS_PER_LIGHT); g_shadowTempInt++) {
        if (fragDepth > currentShadowSource._lightPosition[g_shadowTempInt].w) {
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

    vec4 sc = currentShadowSource._lightVP[g_shadowTempInt] * VAR._vertexW;
    vec4 scPostW = (sc / sc.w) * 0.5 + 0.5;
    if (!(sc.w <= 0 || (scPostW.x < 0 || scPostW.y < 0) || (scPostW.x >= 1 || scPostW.y >= 1))){
        float layer = float(g_shadowTempInt + details.y);

        vec2 moments = texture(texDepthMapFromLightArray, vec3(scPostW.xy, layer)).rg;
       
        //float shadowBias = DEPTH_EXP_WARP * exp(DEPTH_EXP_WARP * dvd_shadowingSettings.y);
        //float shadowWarpedz1 = exp(scPostW.z * DEPTH_EXP_WARP);
        //return mix(chebyshevUpperBound(moments, shadowWarpedz1, dvd_shadowingSettings.y), 
        //             1.0, 
        //             clamp(((gl_FragCoord.z + dvd_shadowingSettings.z) - dvd_shadowingSettings.w) / dvd_shadowingSettings.z, 0.0, 1.0));
        return reduceLightBleeding(chebyshevUpperBound(moments, sc.z, dvd_shadowingSettings.y), 0.2);
        //return VSM(moments, sc.z);
    }
    
    return 1.0;
}
#endif //_SHADOW_DIRECTIONAL_FRAG_