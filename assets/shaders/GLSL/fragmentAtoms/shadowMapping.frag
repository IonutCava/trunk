#ifndef _SHADOW_MAPPING_FRAG_
#define _SHADOW_MAPPING_FRAG_

layout(binding = SHADOW_SINGLE_MAP_ARRAY)  uniform sampler2DArrayShadow    texDepthMapFromLight;
layout(binding = SHADOW_CUBE_MAP_ARRAY)    uniform samplerCubeArrayShadow  texDepthMapFromLightCube;
layout(binding = SHADOW_LAYERED_MAP_ARRAY) uniform sampler2DArray          texDepthMapFromLightArray;


#if defined(_DEBUG)
#define DEBUG_SHADOWMAPPING
// set this to whatever (current cascade, current depth comparison result, anything)
int _shadowTempInt = -2;
#endif

#include "shadow_directional.frag"
#include "shadow_point.frag"
#include "shadow_spot.frag"

float getShadowFactor(int lightIndex) {
    switch (dvd_LightSource[lightIndex]._options.x) {
        case LIGHT_DIRECTIONAL     : return applyShadowDirectional(dvd_LightSource[lightIndex]._options.z);
    //  case LIGHT_OMNIDIRECTIONAL : return applyShadowPoint(dvd_LightSource[lightIndex]._options.z);
    //  case LIGHT_SPOT            : return applyShadowSpot(dvd_LightSource[lightIndex]._options.z); 
    }
}

float shadow_loop(){
    if (!dvd_shadowsEnabled) {
        return 1.0;
    }

    float shadow = 1.0;
    int offset = 0;
    int shadowLights = 0;
    for (int i = 0; i < MAX_LIGHT_TYPES; ++i) {
        for (int j = 0; j < int(dvd_lightCountPerType[i]); ++j) {
            int lightIndex = j + offset;
            if (shadowLights < MAX_SHADOW_CASTING_LIGHTS && 
                dvd_LightSource[lightIndex]._options.y == 1) {
                shadow *= getShadowFactor(lightIndex);
                shadowLights++;
            }
        }
        offset += int(dvd_lightCountPerType[i]);
    }

    return clamp(shadow, 0.05, 1.0);
}
#endif //_SHADOW_MAPPING_FRAG_