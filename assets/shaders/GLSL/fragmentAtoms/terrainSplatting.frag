uniform sampler2D texBlend[MAX_TEXTURE_LAYERS];
uniform sampler2DArray texTileMaps[MAX_TEXTURE_LAYERS];

uniform vec4 diffuseScale[MAX_TEXTURE_LAYERS];
uniform vec4 detailScale[MAX_TEXTURE_LAYERS];

uniform sampler2D texWaterCaustics;
uniform sampler2D texUnderwaterAlbedo;
uniform sampler2D texUnderwaterDetail;
uniform float underwaterDiffuseScale;

vec4 getFinalColor1(const in vec4 blendMap, const in uint index, const in vec4 diffSize) {
    return texture(texTileMaps[index], vec3(_texCoord * diffSize.r, 0)) * blendMap.r;
}

vec3 getFinalTBN1(const in vec4 blendMap, const in uint index, const in vec4 normSize){
    return texture(texTileMaps[index], vec3(_texCoord * normSize.r, 1)).rgb * blendMap.r;
}

vec4 getFinalColor2(const in vec4 blendMap, const in uint index, const in vec4 diffSize){
    vec4 greenChannel = texture(texTileMaps[index], vec3(_texCoord * diffSize.g, 2));
    return mix(getFinalColor1(blendMap, index, diffSize), greenChannel, blendMap.g);
}

vec3 getFinalTBN2(const in vec4 blendMap, const in uint index, const in vec4 normSize){
    vec3 greenChannel = texture(texTileMaps[index], vec3(_texCoord * normSize.g, 3)).rgb;
    return mix(getFinalTBN1(blendMap, index, normSize), greenChannel, blendMap.g);
}

vec4 getFinalColor3(const in vec4 blendMap, const in uint index, const in vec4 diffSize){
    vec4 blueChannel = texture(texTileMaps[index], vec3(_texCoord * diffSize.b, 4));
    return mix(getFinalColor2(blendMap, index, diffSize), blueChannel, blendMap.b);
}

vec3 getFinalTBN3(const in vec4 blendMap, const in uint index, const in vec4 normSize){
    vec3 blueChannel = texture(texTileMaps[index], vec3(_texCoord * normSize.b, 5)).rgb;
    return mix(getFinalTBN2(blendMap, index, normSize), blueChannel, blendMap.b);
}

vec4 getFinalColor4(const in vec4 blendMap, const in uint index, const in vec4 diffSize){
    vec4 alphaChannel = texture(texTileMaps[index], vec3(_texCoord * diffSize.a, 6));
    return mix(getFinalColor3(blendMap, index, diffSize), alphaChannel, blendMap.a);
}

vec3 getFinalTBN4(const in vec4 blendMap, const in uint index, const in vec4 normSize){
    vec3 alphaChannel = texture(texTileMaps[index], vec3(_texCoord * normSize.a, 7)).rgb;

    return mix(getFinalTBN3(blendMap, index, normSize), alphaChannel, blendMap.a);
}

void getColorAndTBNNormal(inout vec4 color, inout vec3 tbn){
    vec4 blendMap;
    color = vec4(0.0);
    tbn = vec3(0.0);
    
    for (uint i = 0; i < MAX_TEXTURE_LAYERS; i++) {
        blendMap = texture(texBlend[i], _texCoord);
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
    color = clamp(color, vec4(0.0, 0.0, 0.0, 1.0), vec4(1.0));
    tbn = normalize((2.0 * tbn - 1.0) / MAX_TEXTURE_LAYERS);
}

void getColorNormal(inout vec4 color){
    vec4 blendMap;
    color = vec4(0.0);

    for (uint i = 0; i < MAX_TEXTURE_LAYERS; i++) {
        blendMap = texture(texBlend[i], _texCoord);
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
}

void getColorAndTBNUnderwater(inout vec4 color, inout vec3 tbn){
    vec2 coords = _texCoord * underwaterDiffuseScale;
    color = texture(texUnderwaterAlbedo, coords);
    tbn = normalize(2.0 * texture(texUnderwaterDetail, coords).rgb - 1.0);
}