#ifndef _SHADOW_MAPPING_FRAG_
#define _SHADOW_MAPPING_FRAG_

layout(binding = SHADOW_SINGLE_MAP_ARRAY)  uniform sampler2DArrayShadow    texDepthMapFromLight;
layout(binding = SHADOW_CUBE_MAP_ARRAY)    uniform samplerCubeArrayShadow  texDepthMapFromLightCube;
layout(binding = SHADOW_LAYERED_MAP_ARRAY) uniform sampler2DArray          texDepthMapFromLightArray;


#if defined(_DEBUG)
#define DEBUG_SHADOWMAPPING
#endif

// set this to whatever (current cascade, current depth comparison result, anything)
int g_shadowTempInt = -2;

#include "shadow_directional.frag"
#include "shadow_point.frag"
#include "shadow_spot.frag"

float getShadowFactor() {

    float shadow = 1.0;
    //uvec4 details[MAX_SHADOW_CASTING_LIGHTS] = dvd_shadowLightDetails;

    for (uint i = 0; i < MAX_SHADOW_CASTING_LIGHTS; ++i) {
        //uvec4 crtDetails = details[i];
        uvec4 crtDetails = dvd_shadowLightDetails[i];

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
        }
    }

    return saturate(shadow);
}

#endif //_SHADOW_MAPPING_FRAG_