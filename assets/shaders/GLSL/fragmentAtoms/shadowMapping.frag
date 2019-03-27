#ifndef _SHADOW_MAPPING_FRAG_
#define _SHADOW_MAPPING_FRAG_

layout(binding = SHADOW_SINGLE_MAP_ARRAY)  uniform sampler2DArrayShadow    texDepthMapFromLight;
layout(binding = SHADOW_CUBE_MAP_ARRAY)    uniform samplerCubeArrayShadow  texDepthMapFromLightCube;
layout(binding = SHADOW_LAYERED_MAP_ARRAY) uniform sampler2DArray          texDepthMapFromLightArray;


#if defined(_DEBUG) || defined(_PROFILE)
#define DEBUG_SHADOWMAPPING
#endif

#if !defined(SHADOW_INTENSITY_FACTOR)
#define SHADOW_INTENSITY_FACTOR 1.0f
#endif

#include "shadow_directional.frag"
#include "shadow_point.frag"
#include "shadow_spot.frag"

float getShadowFactor() {

    float shadow = 1.0;

#if defined(DISABLE_SHADOW_MAPPING)
    return shadow;
#endif

    for (uint i = 0; i < MAX_SHADOW_CASTING_LIGHTS; ++i) {
        const uvec4 crtDetails = dvd_shadowLightDetails[i];

        switch (crtDetails.x) {
            case LIGHT_DIRECTIONAL:
                shadow *= applyShadowDirectional(i, crtDetails);
                break;
            case LIGHT_OMNIDIRECTIONAL:
                shadow *= applyShadowPoint(i, crtDetails);
                break;
            case LIGHT_SPOT:
                shadow *= applyShadowSpot(i, crtDetails);
                break;
            case LIGHT_NONE:
                i = MAX_SHADOW_CASTING_LIGHTS; //quit loop as these should be sorted!
                break;
        }
    }

    return saturate(shadow / SHADOW_INTENSITY_FACTOR);
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