#ifndef _SHADOW_MAPPING_FRAG_
#define _SHADOW_MAPPING_FRAG_

layout(binding = SHADOW_SINGLE_MAP_ARRAY)  uniform sampler2DArrayShadow    texDepthMapFromLight;
layout(binding = SHADOW_CUBE_MAP_ARRAY)    uniform samplerCubeArrayShadow  texDepthMapFromLightCube;
layout(binding = SHADOW_LAYERED_MAP_ARRAY) uniform sampler2DArray          texDepthMapFromLightArray;


#define DEBUG_SHADOWMAPPING
//#define DEBUG_SHADOWSPLITS

// set this to whatever (current cascade, current depth comparison result, anything)
int g_shadowTempInt = -2;

#include "shadow_directional.frag"
#include "shadow_point.frag"
#include "shadow_spot.frag"

float getShadowFactor(in int index, in float fragDepth, in uvec4 lightData) {
    switch (lightData.x) {
        case LIGHT_DIRECTIONAL     : return applyShadowDirectional(index, int(lightData.y), fragDepth);
        /*case LIGHT_OMNIDIRECTIONAL : return applyShadowPoint(index);
        case LIGHT_SPOT            : return applyShadowSpot(index);*/
    }

    return 1.0;
}

float shadow_loop(){
    float shadow = 1.0;
    for (int i = 0; i < TOTAL_SHADOW_LIGHTS; ++i) {
        shadow *= getShadowFactor(i, gl_FragCoord.z, dvd_ShadowSource[i]._lightDetails);
    }

    return saturate(shadow);
}
#endif //_SHADOW_MAPPING_FRAG_