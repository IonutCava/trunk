uniform float dvd_lightBleedBias = 0.001;
uniform float dvd_overDarkFactor = 0.5f;

void applyShadowDirectional(in int shadowIndex, inout float shadow) {
    Light currentLight = dvd_LightSource[dvd_lightIndex[shadowIndex]];
    // find the appropriate depth map to look up in based on the depth of this fragment
    if (gl_FragCoord.z < currentLight._floatValue0)      dvd_shadowMappingTempInt = 0;
    else if (gl_FragCoord.z < currentLight._floatValue1) dvd_shadowMappingTempInt = 1;
    else if (gl_FragCoord.z < currentLight._floatValue2) dvd_shadowMappingTempInt = 2;
    else if (gl_FragCoord.z < currentLight._floatValue3) dvd_shadowMappingTempInt = 3;
    else return;
    
    vec4 shadow_coord = getCoord(currentLight, dvd_shadowMappingTempInt);
    float shadow_factor = texture(texDepthMapFromLightArray, vec3(shadow_coord.xy, dvd_shadowMappingTempInt)).r - (shadow_coord.z - dvd_lightBleedBias);
    shadow = clamp(exp(dvd_overDarkFactor * shadow_factor), 0.0, 1.0);
}