#ifndef _TERRAIN_SPLATTING_FRAG_
#define _TERRAIN_SPLATTING_FRAG_

uniform sampler2D texBlend[MAX_TEXTURE_LAYERS];
uniform sampler2DArray texTileMaps[MAX_TEXTURE_LAYERS];
uniform sampler2DArray texNormalMaps[MAX_TEXTURE_LAYERS];

uniform vec4 diffuseScale[MAX_TEXTURE_LAYERS];
uniform vec4 detailScale[MAX_TEXTURE_LAYERS];

layout(binding = TEXTURE_UNIT0)     uniform sampler2D texWaterCaustics;
layout(binding = TEXTURE_UNIT1)     uniform sampler2D texUnderwaterAlbedo;
layout(binding = TEXTURE_NORMALMAP) uniform sampler2D texUnderwaterDetail;

uniform float underwaterDiffuseScale;

vec4 getFinalColour1(const in vec4 blendMap, const in uint index, const in vec4 diffSize) {
    return texture(texTileMaps[index], vec3(VAR._texCoord * diffSize.r, 0)) * blendMap.r;
}

vec3 getFinalTBN1(const in vec4 blendMap, const in uint index, const in vec4 normSize){
    return texture(texNormalMaps[index], vec3(VAR._texCoord * normSize.r, 0)).rgb * blendMap.r;
}

vec4 getFinalColour2(const in vec4 blendMap, const in uint index, const in vec4 diffSize){
    return mix(getFinalColour1(blendMap, index, diffSize), 
               texture(texTileMaps[index], vec3(VAR._texCoord * diffSize.g, 1)),
               blendMap.g);
}

vec3 getFinalTBN2(const in vec4 blendMap, const in uint index, const in vec4 normSize){
    return mix(getFinalTBN1(blendMap, index, normSize), 
               texture(texNormalMaps[index], vec3(VAR._texCoord * normSize.g, 1)).rgb,
               blendMap.g);
}

vec4 getFinalColour3(const in vec4 blendMap, const in uint index, const in vec4 diffSize){
    return mix(getFinalColour2(blendMap, index, diffSize), 
               texture(texTileMaps[index], vec3(VAR._texCoord * diffSize.b, 2)),
               blendMap.b);
}

vec3 getFinalTBN3(const in vec4 blendMap, const in uint index, const in vec4 normSize){
    return mix(getFinalTBN2(blendMap, index, normSize), 
               texture(texNormalMaps[index], vec3(VAR._texCoord * normSize.b, 2)).rgb,
               blendMap.b);
}

vec4 getFinalColour4(const in vec4 blendMap, const in uint index, const in vec4 diffSize){
    return mix(getFinalColour3(blendMap, index, diffSize), 
               texture(texTileMaps[index], vec3(VAR._texCoord * diffSize.a, 3)),
               blendMap.a);
}

vec3 getFinalTBN4(const in vec4 blendMap, const in uint index, const in vec4 normSize){
    return mix(getFinalTBN3(blendMap, index, normSize), 
               texture(texNormalMaps[index], vec3(VAR._texCoord * normSize.a, 3)).rgb,
               blendMap.a);
}

vec3 getTBNNormal(in vec2 uv, in vec3 normal) {
    mat3 TBN = mat3(VAR._tangentWV, VAR._bitangentWV, VAR._normalWV);
    return TBN * normal;
}

void getColourNormal(inout vec4 colour){
    vec4 blendMap;
    colour = vec4(0.0);

    for (uint i = 0; i < MAX_TEXTURE_LAYERS; i++) {
        blendMap = texture(texBlend[i], VAR._texCoord);
#if (CURRENT_TEXTURE_COUNT % 4) == 1
        colour += getFinalColour1(blendMap, i, diffuseScale[i]);
#elif (CURRENT_TEXTURE_COUNT % 4) == 2
        colour += getFinalColour2(blendMap, i, diffuseScale[i]);
#elif (CURRENT_TEXTURE_COUNT % 4) == 3
        colour += getFinalColour3(blendMap, i, diffuseScale[i]);
#else//(CURRENT_TEXTURE_COUNT % 4) == 0
        colour += getFinalColour4(blendMap, i, diffuseScale[i]);
#endif
    }
}

void getColourAndTBNNormal(inout vec4 colour, inout vec3 tbn) {
    getColourNormal(colour);
    vec4 blendMap = texture(texBlend[0], VAR._texCoord);
#if (CURRENT_TEXTURE_COUNT % 4) == 1
    tbn = getFinalTBN1(blendMap, 0, detailScale[0]);
#elif (CURRENT_TEXTURE_COUNT % 4) == 2
    tbn = getFinalTBN2(blendMap, 0, detailScale[0]);
#elif (CURRENT_TEXTURE_COUNT % 4) == 3
    tbn = getFinalTBN3(blendMap, 0, detailScale[0]);
#else//(CURRENT_TEXTURE_COUNT % 4) == 0
    tbn = getFinalTBN4(blendMap, 0, detailScale[0]);
#endif

    tbn = normalize(2.0 * tbn - 1.0);
    tbn = getTBNNormal(VAR._texCoord, tbn);
}

void getColourAndTBNUnderwater(inout vec4 colour, inout vec3 tbn){
    vec2 coords = VAR._texCoord * underwaterDiffuseScale;
    colour = texture(texUnderwaterAlbedo, coords);
    tbn = normalize(2.0 * texture(texUnderwaterDetail, coords).rgb - 1.0);
    tbn = getTBNNormal(VAR._texCoord, tbn);
}

#endif //_TERRAIN_SPLATTING_FRAG_