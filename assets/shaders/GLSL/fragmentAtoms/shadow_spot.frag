#ifndef _SHADOW_SPOT_FRAG_
#define _SHADOW_SPOT_FRAG_

float filterFinalShadow(in sampler2DArrayShadow depthMap, in vec4 vPosInDM){
    // Gaussian 3x3 filter
    float vDepthMapColour = texture(depthMap, vPosInDM);

    float fShadow = 0.0;
    if((vDepthMapColour+Z_TEST_SIGMA) < vPosInDM.w){
#if GPU_VENDOR == GPU_VENDOR_AMD
        fShadow = vDepthMapColour;
#else
        fShadow = vDepthMapColour * 0.25;
        fShadow += textureOffset(depthMap, vPosInDM, ivec2( -1, -1)) * 0.0625;
        fShadow += textureOffset(depthMap, vPosInDM, ivec2( -1,  0)) * 0.125;
        fShadow += textureOffset(depthMap, vPosInDM, ivec2( -1,  1)) * 0.0625;
        fShadow += textureOffset(depthMap, vPosInDM, ivec2(  0, -1)) * 0.125;
        fShadow += textureOffset(depthMap, vPosInDM, ivec2(  0,  1)) * 0.125;
        fShadow += textureOffset(depthMap, vPosInDM, ivec2(  1, -1)) * 0.0625;
        fShadow += textureOffset(depthMap, vPosInDM, ivec2(  1,  0)) * 0.125;
        fShadow += textureOffset(depthMap, vPosInDM, ivec2(  1,  1)) * 0.0625;
#endif
        fShadow = clamp(fShadow, 0.0, 1.0);
    }else{
        fShadow = 1.0;
    }

    return fShadow;
}

float applyShadowSpot(in uint idx, in uvec4 details, in float fragDepth) {
    Shadow currentShadowSource = dvd_ShadowSource[idx];

    vec4 shadow_coord = currentShadowSource._lightVP[0] * VAR._vertexW;
    shadow_coord = shadow_coord / shadow_coord.w;
    shadow_coord = 0.5 + shadow_coord * 0.5;
    shadow_coord.xyw = shadow_coord.xyz;
    shadow_coord.z = details.y;

    return filterFinalShadow(texDepthMapFromLight, shadow_coord);
}

#endif //_SHADOW_SPOT_FRAG_