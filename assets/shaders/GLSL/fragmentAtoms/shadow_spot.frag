float filterFinalShadow(in sampler2DShadow depthMap, in vec3 vPosInDM){
    
    // Gaussian 3x3 filter
    float vDepthMapColor = texture(depthMap, vPosInDM);
    float fShadow = 0.0;
    if((vDepthMapColor+Z_TEST_SIGMA) < vPosInDM.z){
        fShadow = vDepthMapColor * 0.25;
        fShadow += textureOffset(depthMap, vPosInDM, ivec2( -1, -1)) * 0.0625;
        fShadow += textureOffset(depthMap, vPosInDM, ivec2( -1, 0)) * 0.125;
        fShadow += textureOffset(depthMap, vPosInDM, ivec2( -1, 1)) * 0.0625;
        fShadow += textureOffset(depthMap, vPosInDM, ivec2( 0, -1)) * 0.125;
        fShadow += textureOffset(depthMap, vPosInDM, ivec2( 0, 1)) * 0.125;
        fShadow += textureOffset(depthMap, vPosInDM, ivec2( 1, -1)) * 0.0625;
        fShadow += textureOffset(depthMap, vPosInDM, ivec2( 1, 0)) * 0.125;
        fShadow += textureOffset(depthMap, vPosInDM, ivec2( 1, 1)) * 0.0625;

        fShadow = clamp(fShadow, 0.0, 1.0);
    }else{
        fShadow = 1.0;
    }
    return fShadow;
}

float applyShadowSpot(const in int lightIndex, const in Shadow currentShadowSource) {
    vec4 shadow_coord = currentShadowSource._lightVP0 * _vertexW;
    shadow_coord =  1.0 + shadow_coord * 0.5;
    shadow_coord.xy = shadow_coord.xy / shadow_coord.w;
    return filterFinalShadow(texDepthMapFromLight[lightIndex], shadow_coord.xyz);
}