#ifndef _SHADOW_DIRECTIONAL_FRAG_
#define _SHADOW_DIRECTIONAL_FRAG_

float linstep(float a, float b, float v) {
    return clamp((v - a) / (b - a), 0.0, 1.0);
}

// Reduces VSM light bleeding (remove the [0, amount] tail and linearly rescale (amount, 1])
float reduceLightBleeding(float pMax, float amount) {
    return linstep(amount, 1.0f, pMax);
}

float chebyshevUpperBound(vec2 moments, float compare, float minVariance) {
    float p = step(compare, moments.x);
    float variance = max(moments.y - (moments.x * moments.x), minVariance);
    
    float d = (compare - moments.x);

    float pMax = reduceLightBleeding(variance / (variance + d*d), dvd_lightBleedBias);
    
    return min(max(p, pMax), 1.0);
}

bool inRange(const in float value){
    return value >= 0.0 && value <= 1.0;
}

float applyShadowDirectional(const in uint lightIndex, const in Shadow currentShadowSource) {

#   if !defined(_DEBUG)
      int _shadowTempInt = -1;
#   endif

    // find the appropriate depth map to look up in based on the depth of this fragment
    if (gl_FragCoord.z < currentShadowSource._floatValues.x)      {
        _shadowTempInt = 0;
    }else if (gl_FragCoord.z < currentShadowSource._floatValues.y) {
        _shadowTempInt = 1;
    }else if (gl_FragCoord.z < currentShadowSource._floatValues.z) {
        _shadowTempInt = 2;
    }else /*(gl_FragCoord.z < currentShadowSource._floatValues.w)*/{
        _shadowTempInt = 3;
    }

    // GLOBAL
    const int SplitPowLookup[8] = { 0, 1, 1, 2, 2, 2, 2, 3 };
    // Ensure that every fragment in the quad choses the same split so that derivatives
    // will be meaningful for proper texture filtering and LOD selection.
    int SplitPow = 1 << _shadowTempInt;
    int SplitX = int(abs(dFdx(SplitPow)));
    int SplitY = int(abs(dFdy(SplitPow)));
    int SplitXY = int(abs(dFdx(SplitY)));
    int SplitMax = max(SplitXY, max(SplitX, SplitY));
    _shadowTempInt = SplitMax > 0 ? SplitPowLookup[SplitMax - 1] : _shadowTempInt;

    if (_shadowTempInt < 0 || _shadowTempInt > 3) {
        return 1.0;
    }

    vec4 shadow_coord = currentShadowSource._lightVP[_shadowTempInt] * _vertexW;
    
    if (inRange(shadow_coord.z) && inRange(shadow_coord.x) && inRange(shadow_coord.y)){
        shadow_coord.w = shadow_coord.z;
        shadow_coord.z = _shadowTempInt;

        vec2 moments = getArrayShadowValue(lightIndex, shadow_coord.xyz);
       
        //float shadowBias = DEPTH_EXP_WARP * exp(DEPTH_EXP_WARP * dvd_minShadowVariance);
        //float shadowWarpedz1 = exp(shadow_coord.w * DEPTH_EXP_WARP);
        //return mix(chebyshevUpperBound(moments, shadowWarpedz1, dvd_minShadowVariance), 1.0, clamp(((gl_FragCoord.z + dvd_shadowFadeDist) - dvd_shadowMaxDist) / dvd_shadowFadeDist, 0.0, 1.0));
        return chebyshevUpperBound(moments,  shadow_coord.w, dvd_minShadowVariance);
    }

    return 1.0;
}

#endif //_SHADOW_DIRECTIONAL_FRAG_