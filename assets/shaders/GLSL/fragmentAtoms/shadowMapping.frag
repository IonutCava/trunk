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

float shadow_loop() {

    float shadow = 1.0;
    float fragDepth = VAR._vertexWV.z;

    for (uint i = 0; i < MAX_SHADOW_CASTING_LIGHTS; ++i) {
        uvec4 details = dvd_shadowLightDetails[i];

        switch (details.x) {
            case LIGHT_DIRECTIONAL:
                shadow *= applyShadowDirectional(i, details, fragDepth);
                break;
            case LIGHT_OMNIDIRECTIONAL:
                shadow *= applyShadowPoint(i, details, fragDepth);
                break;
            case LIGHT_SPOT:
                shadow *= applyShadowSpot(i, details, fragDepth);
                break;
        }
    }

    return saturate(shadow);
}
#endif //_SHADOW_MAPPING_FRAG_