uniform float dvd_lightBleedBias = 0.0001;
uniform float dvd_shadowMaxDist = 300.0;
uniform float dvd_shadowFadeDist = 200.0;

float linstep(float a, float b, float v) {
    return clamp((v - a) / (b - a), 0.0, 1.0);
}

// Reduces VSM light bleeding (remove the [0, amount] tail and linearly rescale (amount, 1])
float reduceLightBleeding(float pMax, float amount) {
    return linstep(amount, 1.0f, pMax);
}

float chebyshevUpperBound(in vec4 posInDM)
{
    vec2 moments = texture(texDepthMapFromLightArray, vec3(posInDM.xy, _shadowTempInt)).rg;
    float distance = (posInDM.z / posInDM.w);
    // Surface is fully lit. as the current fragment is before the light occluder
    if (distance <= moments.x)
        return 1.0;

    // The fragment is either in shadow or penumbra. We now use chebyshev's upperBound to check
    // How likely this pixel is to be lit (p_max)
    float variance = moments.y - (moments.x*moments.x);
    variance = max(variance, 0.00002);

    float d = distance - moments.x;
    float p_max = variance / (variance + d*d);
    return reduceLightBleeding(p_max, dvd_lightBleedBias);
}

void applyShadowDirectional(in int shadowIndex, inout float shadow) {
    float currentDistance = gl_FragCoord.z;
    Light currentLight = dvd_LightSource[dvd_lightIndex[shadowIndex]];
    vec4 shadow_coord;
    // find the appropriate depth map to look up in based on the depth of this fragment
    if (currentDistance < currentLight._floatValue0)        {
        _shadowTempInt = 0;
        shadow_coord = currentLight._lightVP0 * _vertexW;
    }else if (currentDistance < currentLight._floatValue1) {
        _shadowTempInt = 1;
        shadow_coord = currentLight._lightVP1 * _vertexW;
    }else if (currentDistance < currentLight._floatValue2) {
        _shadowTempInt = 2;
        shadow_coord = currentLight._lightVP2 * _vertexW;
    }else if (currentDistance < currentLight._floatValue3) {
        _shadowTempInt = 3;
        shadow_coord = currentLight._lightVP3 * _vertexW;
    }else return;
    
    if (shadow_coord.w > 0.0){
        shadow = mix(chebyshevUpperBound(shadow_coord), 1.0, clamp(((currentDistance + dvd_shadowFadeDist) - dvd_shadowMaxDist) / dvd_shadowFadeDist, 0.0, 1.0));
    }
}





