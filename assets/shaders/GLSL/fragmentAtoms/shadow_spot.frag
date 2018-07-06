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

float applyShadowSpot(const in uint lightIndex, const in Shadow currentShadowSource) {
    vec4 shadow_coord = currentShadowSource._lightVP[0] * _vertexW;
    shadow_coord =  1.0 + shadow_coord * 0.5;
    shadow_coord.xy = shadow_coord.xy / shadow_coord.w;
#   if MAX_SHADOW_CASTING_LIGHTS > 1
        switch(lightIndex){
            case 0 :  return filterFinalShadow(texDepthMapFromLight[0], shadow_coord.xyz);
            case 1 :  return filterFinalShadow(texDepthMapFromLight[1], shadow_coord.xyz);
            #if MAX_SHADOW_CASTING_LIGHTS > 2
            case 2 :  return filterFinalShadow(texDepthMapFromLight[2], shadow_coord.xyz);
            #if MAX_SHADOW_CASTING_LIGHTS > 3
            case 3 :  return filterFinalShadow(texDepthMapFromLight[3], shadow_coord.xyz);
            #endif
            #endif
        };
#   else
        return filterFinalShadow(texDepthMapFromLight[0], shadow_coord.xyz);
#   endif
}