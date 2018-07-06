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

vec4 getFinalColor1(const in vec4 blendMap, const in uint index, const in vec4 diffSize) {
    return texture(texTileMaps[index], vec3(VAR._texCoord * diffSize.r, 0)) * blendMap.r;
}

vec3 getFinalTBN1(const in vec4 blendMap, const in uint index, const in vec4 normSize){
    return texture(texNormalMaps[index], vec3(VAR._texCoord * normSize.r, 0)).rgb * blendMap.r;
}

vec4 getFinalColor2(const in vec4 blendMap, const in uint index, const in vec4 diffSize){
    return mix(getFinalColor1(blendMap, index, diffSize), 
               texture(texTileMaps[index], vec3(VAR._texCoord * diffSize.g, 1)),
               blendMap.g);
}

vec3 getFinalTBN2(const in vec4 blendMap, const in uint index, const in vec4 normSize){
    return mix(getFinalTBN1(blendMap, index, normSize), 
               texture(texNormalMaps[index], vec3(VAR._texCoord * normSize.g, 1)).rgb,
               blendMap.g);
}

vec4 getFinalColor3(const in vec4 blendMap, const in uint index, const in vec4 diffSize){
    return mix(getFinalColor2(blendMap, index, diffSize), 
               texture(texTileMaps[index], vec3(VAR._texCoord * diffSize.b, 2)),
               blendMap.b);
}

vec3 getFinalTBN3(const in vec4 blendMap, const in uint index, const in vec4 normSize){
    return mix(getFinalTBN2(blendMap, index, normSize), 
               texture(texNormalMaps[index], vec3(VAR._texCoord * normSize.b, 2)).rgb,
               blendMap.b);
}

vec4 getFinalColor4(const in vec4 blendMap, const in uint index, const in vec4 diffSize){
    return mix(getFinalColor3(blendMap, index, diffSize), 
               texture(texTileMaps[index], vec3(VAR._texCoord * diffSize.a, 3)),
               blendMap.a);
}

vec3 getFinalTBN4(const in vec4 blendMap, const in uint index, const in vec4 normSize){
    return mix(getFinalTBN3(blendMap, index, normSize), 
               texture(texNormalMaps[index], vec3(VAR._texCoord * normSize.a, 3)).rgb,
               blendMap.a);
}

void getColorAndTBNNormal(inout vec4 color, inout vec3 tbn){
    vec4 blendMap;
    color = vec4(0.0);
    tbn = vec3(0.0);
    
    for (uint i = 0; i < MAX_TEXTURE_LAYERS; i++) {
        blendMap = texture(texBlend[i], VAR._texCoord);
#if (CURRENT_TEXTURE_COUNT % 4) == 1
        color += getFinalColor1(blendMap, i, diffuseScale[i]);
        tbn   += getFinalTBN1(blendMap, i, detailScale[i]);
#elif (CURRENT_TEXTURE_COUNT % 4) == 2
        color += getFinalColor2(blendMap, i, diffuseScale[i]);
        tbn   += getFinalTBN2(blendMap, i, detailScale[i]);
#elif (CURRENT_TEXTURE_COUNT % 4) == 3
        color += getFinalColor3(blendMap, i, diffuseScale[i]);
        tbn   += getFinalTBN3(blendMap, i, detailScale[i]);
#else//(CURRENT_TEXTURE_COUNT % 4) == 0
        color += getFinalColor4(blendMap, i, diffuseScale[i]);
        tbn   += getFinalTBN4(blendMap, i, detailScale[i]);
#endif
    }
    color = clamp(color, vec4(0.0), vec4(1.0));
    tbn = normalize((2.0 * tbn - 1.0) / MAX_TEXTURE_LAYERS);
}

void getColorNormal(inout vec4 color){
    vec4 blendMap;
    color = vec4(0.0);

    for (uint i = 0; i < MAX_TEXTURE_LAYERS; i++) {
        blendMap = texture(texBlend[i], VAR._texCoord);
#if (CURRENT_TEXTURE_COUNT % 4) == 1
        color += getFinalColor1(blendMap, i, diffuseScale[i]);
#elif (CURRENT_TEXTURE_COUNT % 4) == 2
        color += getFinalColor2(blendMap, i, diffuseScale[i]);
#elif (CURRENT_TEXTURE_COUNT % 4) == 3
        color += getFinalColor3(blendMap, i, diffuseScale[i]);
#else//(CURRENT_TEXTURE_COUNT % 4) == 0
        color += getFinalColor4(blendMap, i, diffuseScale[i]);
#endif
    }
    color = clamp(color, vec4(0.0), vec4(1.0));
}

void getColorAndTBNUnderwater(inout vec4 color, inout vec3 tbn){
    vec2 coords = VAR._texCoord * underwaterDiffuseScale;
    color = texture(texUnderwaterAlbedo, coords);
    tbn = normalize(2.0 * texture(texUnderwaterDetail, coords).rgb - 1.0);
}

#endif //_TERRAIN_SPLATTING_FRAG_