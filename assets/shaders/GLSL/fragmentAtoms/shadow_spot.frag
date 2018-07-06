#ifndef _SHADOW_SPOT_FRAG_
#define _SHADOW_SPOT_FRAG_

float compareResult(in float value, in float compare) {
    return step(value, compare);
}

float filterFinalShadow(in sampler2DArray depthMap, in vec4 vPosInDM){
    // Gaussian 3x3 filter
    float vDepthMapColor = compareResult(texture(depthMap, vPosInDM.xyz).r, vPosInDM.w);

    float fShadow = 0.0;
    if((vDepthMapColor+Z_TEST_SIGMA) < vPosInDM.z){
        fShadow = vDepthMapColor * 0.25;
        fShadow += compareResult(textureOffset(depthMap, vPosInDM.xyz, ivec2( -1, -1)).r, vPosInDM.w) * 0.0625;
        fShadow += compareResult(textureOffset(depthMap, vPosInDM.xyz, ivec2( -1,  0)).r, vPosInDM.w)  * 0.125;
        fShadow += compareResult(textureOffset(depthMap, vPosInDM.xyz, ivec2( -1,  1)).r, vPosInDM.w)  * 0.0625;
        fShadow += compareResult(textureOffset(depthMap, vPosInDM.xyz, ivec2(  0, -1)).r, vPosInDM.w) * 0.125;
        fShadow += compareResult(textureOffset(depthMap, vPosInDM.xyz, ivec2(  0,  1)).r, vPosInDM.w)  * 0.125;
        fShadow += compareResult(textureOffset(depthMap, vPosInDM.xyz, ivec2(  1, -1)).r, vPosInDM.w) * 0.0625;
        fShadow += compareResult(textureOffset(depthMap, vPosInDM.xyz, ivec2(  1,  0)).r, vPosInDM.w)  * 0.125;
        fShadow += compareResult(textureOffset(depthMap, vPosInDM.xyz, ivec2(  1,  1)).r, vPosInDM.w)  * 0.0625;

        fShadow = clamp(fShadow, 0.0, 1.0);
    }else{
        fShadow = 1.0;
    }

    return fShadow;
}

float applyShadowSpot(int shadowIndex) {
    Shadow currentShadowSource = dvd_ShadowSource[shadowIndex];

    vec4 shadow_coord = currentShadowSource._lightVP[0] * VAR._vertexW;
    shadow_coord =  1.0 + shadow_coord * 0.5;
    shadow_coord.xy = shadow_coord.xy / shadow_coord.w;

    vec4 texcoord;
    texcoord.xyw = shadow_coord.xyz;
    texcoord.z = currentShadowSource._arrayOffset.x;

    return filterFinalShadow(texDepthMapFromLight, texcoord);
}

#endif //_SHADOW_SPOT_FRAG_