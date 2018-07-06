uniform float dvd_lightBleedBias = 0.0001;
uniform float dvd_minShadowVariance = 0.00002;
uniform float dvd_overDarkValue = 1.5;
uniform float dvd_shadowMaxDist = 250.0;
uniform float dvd_shadowFadeDist = 150.0;

float linstep(float a, float b, float v) {
    return clamp((v - a) / (b - a), 0.0, 1.0);
}

// Reduces VSM light bleeding (remove the [0, amount] tail and linearly rescale (amount, 1])
float reduceLightBleeding(float pMax, float amount) {
    return linstep(amount, 1.0f, pMax);
}

float chebyshevUpperBound(vec2 moments, float t, float minVariance)
{
    float p = (t <= moments.x) ? 1.0 : 0.0;
    float variance = moments.y - (moments.x * moments.x);
    variance = max(variance, minVariance);
    float d = (t - moments.x) * dvd_overDarkValue;
    float p_max = variance / (variance + d*d);
    return reduceLightBleeding(max(p, p_max), dvd_lightBleedBias);
}

void applyShadowDirectional(in int shadowIndex, inout float shadow) {

    // GLOBAL
    const int SplitPowLookup[8] = { 0, 1, 1, 2, 2, 2, 2, 3 };

    float currentDistance = gl_FragCoord.z;
    Shadow currentShadowSource = dvd_ShadowSource[dvd_lightIndex[shadowIndex]];
    vec4 shadow_coord;
    // find the appropriate depth map to look up in based on the depth of this fragment
    if (currentDistance < currentShadowSource._floatValues.x)      {
        _shadowTempInt = 0;
        shadow_coord = currentShadowSource._lightVP0 * _vertexW;
    }else if (currentDistance < currentShadowSource._floatValues.y) {
        _shadowTempInt = 1;
        shadow_coord = currentShadowSource._lightVP1 * _vertexW;
    }else if (currentDistance < currentShadowSource._floatValues.z) {
        _shadowTempInt = 2;
        shadow_coord = currentShadowSource._lightVP2 * _vertexW;
    }else /*(currentDistance < currentShadowSource._floatValues.w)*/{
        _shadowTempInt = 3;
        shadow_coord = currentShadowSource._lightVP3 * _vertexW;
    }
    
    // Ensure that every fragment in the quad choses the same split so that
    // derivatives will be meaningful for proper texture filtering and LOD
    // selection.
    int SplitPow = 1 << _shadowTempInt;
    int SplitX   = int (abs(dFdx(SplitPow)));
    int SplitY   = int (abs(dFdy(SplitPow)));
    int SplitXY  = int (abs(dFdx(SplitY)));
    int SplitMax = max(SplitXY, max(SplitX, SplitY));
    _shadowTempInt = SplitMax > 0 ? SplitPowLookup[SplitMax - 1] : _shadowTempInt;

    shadow_coord.w = shadow_coord.z * 0.5 + 0.5;
    shadow_coord.z = _shadowTempInt;
    if (shadow_coord.w > 0.0){
        vec2 moments = texture(texDepthMapFromLightArray, shadow_coord.xyz).rg;
        //float shadowBias = DEPTH_EXP_WARP * exp(DEPTH_EXP_WARP * dvd_minShadowVariance);
        //float shadowWarpedz1 = exp(shadow_coord.w * DEPTH_EXP_WARP);
        float shadowWarpedz1 = shadow_coord.w;
        //shadow = mix(chebyshevUpperBound(moments, shadowWarpedz1, dvd_minShadowVariance), 1.0, clamp(((currentDistance + dvd_shadowFadeDist) - dvd_shadowMaxDist) / dvd_shadowFadeDist, 0.0, 1.0));
        shadow = chebyshevUpperBound(moments, shadowWarpedz1, dvd_minShadowVariance);
    }
}





