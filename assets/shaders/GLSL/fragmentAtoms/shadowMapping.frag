#ifndef _SHADOW_MAPPING_FRAG_
#define _SHADOW_MAPPING_FRAG_

layout(binding = SHADOW_CUBE_START)   uniform samplerCubeShadow texDepthMapFromLightCube[MAX_SHADOW_CASTING_LIGHTS];
layout(binding = SHADOW_NORMAL_START) uniform sampler2DShadow   texDepthMapFromLight[MAX_SHADOW_CASTING_LIGHTS];
layout(binding = SHADOW_ARRAY_START)  uniform sampler2DArray    texDepthMapFromLightArray[MAX_SHADOW_CASTING_LIGHTS];

// dynamic indexing doesn't work for some reason
vec2 getArrayShadowValue(in uint index, in vec3 coords){
    #if MAX_SHADOW_CASTING_LIGHTS > 1
        switch(index){
            case 0 :  return texture(texDepthMapFromLightArray[0], coords).rg;
            case 1 :  return texture(texDepthMapFromLightArray[1], coords).rg;
            #if MAX_SHADOW_CASTING_LIGHTS > 2
            case 2 :  return texture(texDepthMapFromLightArray[2], coords).rg;
            #if MAX_SHADOW_CASTING_LIGHTS > 3
            case 3 :  return texture(texDepthMapFromLightArray[3], coords).rg;
            #endif
            #endif
        };
    #else
        return texture(texDepthMapFromLightArray[0], coords).rg
    #endif
}

float getCubeShadowValue(in uint index, in vec4 coords){
    #if MAX_SHADOW_CASTING_LIGHTS > 1
        switch(index){
            case 0 :  return texture(texDepthMapFromLightCube[0], coords).r;
            case 1 :  return texture(texDepthMapFromLightCube[1], coords).r;
            #if MAX_SHADOW_CASTING_LIGHTS > 2
            case 2 :  return texture(texDepthMapFromLightCube[2], coords).r;
            #if MAX_SHADOW_CASTING_LIGHTS > 3
            case 3 :  return texture(texDepthMapFromLightCube[3], coords).r;
            #endif
            #endif
        };
    #else
        return texture(texDepthMapFromLightCube[0], coords).r
    #endif
}

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
                    shadow *= applyShadowDirectional(
                        i, dvd_ShadowSource[dvd_LightSource[i]._options.z]);
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