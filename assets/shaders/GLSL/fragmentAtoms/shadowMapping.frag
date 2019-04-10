#ifndef _SHADOW_MAPPING_FRAG_
#define _SHADOW_MAPPING_FRAG_

layout(binding = SHADOW_SINGLE_MAP_ARRAY)  uniform sampler2DArrayShadow    texDepthMapFromLight;
layout(binding = SHADOW_CUBE_MAP_ARRAY)    uniform samplerCubeArrayShadow  texDepthMapFromLightCube;
layout(binding = SHADOW_LAYERED_MAP_ARRAY) uniform sampler2DArray          texDepthMapFromLightArray;

layout(binding = TEXTURE_PREPASS_SHADOWS)  uniform sampler2D               texDepthMapFromPrePass;



#if defined(_DEBUG) || defined(_PROFILE)
#define DEBUG_SHADOWMAPPING
#endif

#if !defined(SHADOW_INTENSITY_FACTOR)
#define SHADOW_INTENSITY_FACTOR 1.0f
#endif

#include "shadow_directional.frag"
#include "shadow_point.frag"
#include "shadow_spot.frag"

float getShadowFactorInternal(int idx) {

#if !defined(PRE_PASS)
    if (idx >= 0 && idx < 4) {
        return texture(texDepthMapFromPrePass, getScreenPositionNormalised())[idx];
    }
#endif

    if (idx >= 0 && idx < MAX_SHADOW_CASTING_LIGHTS) {
        uint lightIndex = uint(idx);

        const uvec4 crtDetails = dvd_shadowLightDetails[idx];
        switch (crtDetails.x) {
            case LIGHT_DIRECTIONAL: return applyShadowDirectional(lightIndex, crtDetails);
            case LIGHT_OMNIDIRECTIONAL: return applyShadowPoint(lightIndex, crtDetails);
            case LIGHT_SPOT: return applyShadowSpot(lightIndex, crtDetails);
        };
    }
    return 1.0f;
}

float getShadowFactor(int idx) {

#if defined(DISABLE_SHADOW_MAPPING)
    return 1.0f;
#else
    return saturate(getShadowFactorInternal(idx) / SHADOW_INTENSITY_FACTOR);
#endif
}

int getShadowData() {

    for (uint i = 0; i < MAX_SHADOW_CASTING_LIGHTS; ++i) {
        const uvec4 crtDetails = dvd_shadowLightDetails[i];
        switch (crtDetails.x) {
            case LIGHT_DIRECTIONAL: return getCSMSlice(i);
        }
    }

    return -2;
}
#endif //_SHADOW_MAPPING_FRAG_