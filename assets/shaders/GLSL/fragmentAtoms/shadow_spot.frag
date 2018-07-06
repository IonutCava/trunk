/*
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
}*/

void applyShadowSpot(in int shadowIndex, inout float shadow) {
    vec3 vPixPosInDepthMap = ((_shadowCoord[shadowIndex].xyz / _shadowCoord[shadowIndex].w) + 1.0) * 0.5;
    //shadow = filterFinalShadow(texDepthMapFromLight[shadowIndex], vPixPosInDepthMap);

    shadow = texture(texDepthMapFromLight[shadowIndex], vPixPosInDepthMap);
}