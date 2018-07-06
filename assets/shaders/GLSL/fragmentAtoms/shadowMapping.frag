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


float shadow_loop(){
    if (!dvd_shadowsEnabled) {
        return 1.0;
    }

    float shadow = 1.0;
    for (uint i = 0; i < MAX_SHADOW_CASTING_LIGHTS; i++) {
        if (_lightCount == i) {
            break;
        }
        if (dvd_LightSource[i]._options.y == 1)
            switch (dvd_LightSource[i]._options.x) {
                case LIGHT_DIRECTIONAL:
                    shadow *= applyShadowDirectional(i, dvd_ShadowSource[dvd_LightSource[i]._options.z]);
                    break;
                    // case LIGHT_OMNIDIRECTIONAL : shadow *= applyShadowPoint(
                    //    i, dvd_ShadowSource[dvd_LightSource[i]._options.z]);
                    // break;
                    // case LIGHT_SPOT            : shadow *= applyShadowSpot(
                    //    i, dvd_ShadowSource[dvd_LightSource[i]._options.z]); 
                    // break;
            }
    }

    return clamp(shadow, 0.2, 1.0);
}
#endif //_SHADOW_MAPPING_FRAG_