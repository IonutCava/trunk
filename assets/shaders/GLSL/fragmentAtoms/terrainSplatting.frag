#ifndef _TERRAIN_SPLATTING_FRAG_
#define _TERRAIN_SPLATTING_FRAG_

#include "bumpMapping.frag"

uniform vec4 diffuseScale[MAX_TEXTURE_LAYERS];
uniform vec4 detailScale[MAX_TEXTURE_LAYERS];

layout(binding = TEXTURE_UNIT0)     uniform sampler2D texWaterCaustics;
layout(binding = TEXTURE_UNIT1)     uniform sampler2D texUnderwaterAlbedo;
layout(binding = TEXTURE_NORMALMAP) uniform sampler2D texUnderwaterDetail;
layout(binding = TEXTURE_OPACITY)   uniform sampler2D texHeightMap;

layout(binding = TEXTURE_COUNT + 0) uniform sampler2DArray texBlendMaps;
layout(binding = TEXTURE_COUNT + 1) uniform sampler2DArray texTileMaps;
layout(binding = TEXTURE_COUNT + 2) uniform sampler2DArray texNormalMaps;

vec4 getFinalColour1(const in vec4 blendMap, const in uint index, const in vec4 diffSize) {
    return texture(texTileMaps, vec3(scaledTextureCoords(VAR._texCoord, diffSize.r), 0 + index));
}

vec3 getFinalTBN1(const in vec4 blendMap, const in uint index, const in vec4 normSize){
    return texture(texNormalMaps, vec3(scaledTextureCoords(VAR._texCoord, normSize.r), 0 + index)).rgb;
}

vec4 getFinalColour2(const in vec4 blendMap, const in uint index, const in vec4 diffSize){
    return mix(getFinalColour1(blendMap, index, diffSize), 
               texture(texTileMaps, vec3(scaledTextureCoords(VAR._texCoord, diffSize.g), 1 + index)),
               blendMap.g);
}

vec3 getFinalTBN2(const in vec4 blendMap, const in uint index, const in vec4 normSize){
    return mix(getFinalTBN1(blendMap, index, normSize), 
               texture(texNormalMaps, vec3(scaledTextureCoords(VAR._texCoord, normSize.g), 1 + index)).rgb,
               blendMap.g);
}

vec4 getFinalColour3(const in vec4 blendMap, const in uint index, const in vec4 diffSize){
    return mix(getFinalColour2(blendMap, index, diffSize), 
               texture(texTileMaps, vec3(scaledTextureCoords(VAR._texCoord, diffSize.b), 2 + index)),
               blendMap.b);
}

vec3 getFinalTBN3(const in vec4 blendMap, const in uint index, const in vec4 normSize){
    return mix(getFinalTBN2(blendMap, index, normSize), 
               texture(texNormalMaps, vec3(scaledTextureCoords(VAR._texCoord, normSize.b), 2 + index)).rgb,
               blendMap.b);
}

vec4 getFinalColour4(const in vec4 blendMap, const in uint index, const in vec4 diffSize){
    return mix(getFinalColour3(blendMap, index, diffSize), 
               texture(texTileMaps, vec3(scaledTextureCoords(VAR._texCoord, diffSize.a), 3 + index)),
               blendMap.a);
}

vec3 getFinalTBN4(const in vec4 blendMap, const in uint index, const in vec4 normSize){
    return mix(getFinalTBN3(blendMap, index, normSize), 
               texture(texNormalMaps, vec3(scaledTextureCoords(VAR._texCoord, normSize.a), 3 + index)).rgb,
               blendMap.a);
}

vec4 getTerrainAlbedo(){
    vec4 blendMap;
    vec4 colour = vec4(0.0);
    
    for (uint i = 0; i < MAX_TEXTURE_LAYERS; i++) {
        blendMap = texture(texBlendMaps, vec3(VAR._texCoord, i));

        uint currentCount = CURRENT_LAYER_COUNT[i];

        if (currentCount == 1) {
            colour += getFinalColour1(blendMap, i * 1, diffuseScale[i * 1]);
        } else if (currentCount == 2) {
            colour += getFinalColour2(blendMap, i * 2, diffuseScale[i * 2]);
        } else if (currentCount == 3) {
            colour += getFinalColour3(blendMap, i * 3, diffuseScale[i * 3]);
        } else {
            colour += getFinalColour4(blendMap, i * 4, diffuseScale[i * 4]);
        }
    }

    return colour;
}

vec3 getTerrainNormal() {

    vec3 V = VAR._vertexW.xyz - dvd_cameraPosition.xyz;

    vec3 tbn = vec3(0.0);
    vec3 tbnTemp;
    for (uint i = 0; i < MAX_TEXTURE_LAYERS; i++) {
        vec4 blendMap = texture(texBlendMaps, vec3(VAR._texCoord, i));

        uint currentCount = CURRENT_LAYER_COUNT[i];
        if (currentCount == 1) {
            tbnTemp = getFinalTBN1(blendMap, i * 1, detailScale[i * 1]);
        } else if (currentCount == 2) {
            tbnTemp = getFinalTBN2(blendMap, i * 2, detailScale[i * 2]);
        } else if (currentCount == 3) {
            tbnTemp = getFinalTBN3(blendMap, i * 3, detailScale[i * 3]);
        } else {
            tbnTemp = getFinalTBN4(blendMap, i * 4, detailScale[i * 4]);
        }

        tbnTemp = perturb_normal(tbnTemp, VAR._normalWV, V, VAR._texCoord);
        tbn = normalUDNBlend(tbnTemp, tbn);
    }

    return tbn;
}

#endif //_TERRAIN_SPLATTING_FRAG_