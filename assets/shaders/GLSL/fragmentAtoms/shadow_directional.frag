#ifndef _SHADOW_DIRECTIONAL_FRAG_
#define _SHADOW_DIRECTIONAL_FRAG_

#define LT(n) n, n, n, n, n, n, n, n, n, n, n, n, n, n, n, n

float linstep(float min, float max, float v) {
    return clamp((v - min) / (max - min), 0.0, 1.0);
}

// Reduces VSM light bleeding (remove the [0, amount] tail and linearly rescale (amount, 1])
float reduceLightBleeding(float pMax, float amount) {
    return linstep(amount, 1.0f, pMax);
}

float chebyshevUpperBound(vec2 moments, float depth, float minVariance) {
    if (depth <= moments.x) {
        return 1.0;
    }
    float variance = max(moments.y - (moments.x * moments.x), minVariance);
    float d = (depth - moments.x);
    return variance / (variance + d*d);
}

float applyShadowDirectional(int shadowIndex, int splitCount) {

    Shadow currentShadowSource  = dvd_ShadowSource[shadowIndex];
#   if !defined(_DEBUG)
      int _shadowTempInt = -2;
#   endif
    float zDist = gl_FragCoord.z;
    
    // find the appropriate depth map to look up in based on the depth of this fragment
    _shadowTempInt = 0;
    for (int i = 0; i < splitCount; ++i) {
        if (zDist > currentShadowSource._floatValues[i].x)      {
            _shadowTempInt = i + 1;
        }
    }
    
    // GLOBAL
    const int SplitPowLookup[] = { 0, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3,
                                   LT(4), LT(5), LT(5), LT(6), LT(6), LT(6), LT(6),
                                   LT(7), LT(7), LT(7), LT(7), LT(7), LT(7), LT(7), LT(7)};
    // Ensure that every fragment in the quad choses the same split so that derivatives
    // will be meaningful for proper texture filtering and LOD selection.
    int SplitPow = 1 << _shadowTempInt;
    int SplitX = int(abs(dFdx(SplitPow)));
    int SplitY = int(abs(dFdy(SplitPow)));
    int SplitXY = int(abs(dFdx(SplitY)));
    int SplitMax = max(SplitXY, max(SplitX, SplitY));
    _shadowTempInt = SplitMax > 0 ? SplitPowLookup[SplitMax - 1] : _shadowTempInt;

    if (_shadowTempInt < 0 || _shadowTempInt > splitCount) {
        return 1.0;
    }

    vec4 sc = currentShadowSource._lightVP[_shadowTempInt] * VAR._vertexW;
    vec4 scPostW = sc / sc.w;

    if (!(sc.w <= 0.0f || (scPostW.x < 0 || scPostW.y < 0) || (scPostW.x >= 1 || scPostW.y >= 1))){
        float layer = float(_shadowTempInt + currentShadowSource._arrayOffset.x);

        vec2 moments = texture(texDepthMapFromLightArray, vec3(scPostW.xy, layer)).rg;
       
        //float shadowBias = DEPTH_EXP_WARP * exp(DEPTH_EXP_WARP * dvd_shadowingSettings.y);
        //float shadowWarpedz1 = exp(scPostW.z * DEPTH_EXP_WARP);
        //return mix(chebyshevUpperBound(moments, shadowWarpedz1, dvd_shadowingSettings.y), 
        //             1.0, 
        //             clamp(((gl_FragCoord.z + dvd_shadowingSettings.z) - dvd_shadowingSettings.w) / dvd_shadowingSettings.z, 0.0, 1.0));
        return reduceLightBleeding(chebyshevUpperBound(moments, scPostW.z, dvd_shadowingSettings.y), 0.8);
    }

    return 1.0;
}

#endif //_SHADOW_DIRECTIONAL_FRAG_