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

float getShadowFactor(in uint index, in float fragDepth) {
    Shadow currentShadowSource = dvd_ShadowSource[index];
    uvec4 lightDetails = currentShadowSource._lightDetails;

    switch (lightDetails.x) {
        case LIGHT_DIRECTIONAL     : return applyShadowDirectional(currentShadowSource, fragDepth);
        case LIGHT_OMNIDIRECTIONAL : return applyShadowPoint(currentShadowSource, fragDepth);
        case LIGHT_SPOT            : return applyShadowSpot(currentShadowSource, fragDepth);
    }

    return 1.0;
}

float shadow_loop(){
    float shadow = 1.0;
    float fragDepth = VAR._vertexWV.z;
    for (uint i = 0; i < TOTAL_SHADOW_LIGHTS; ++i) {
        shadow *= getShadowFactor(i, fragDepth);
    }

    return saturate(shadow);
}
#endif //_SHADOW_MAPPING_FRAG_