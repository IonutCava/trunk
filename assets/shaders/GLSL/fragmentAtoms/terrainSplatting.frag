uniform sampler2D texBlend[MAX_TEXTURE_LAYERS];
uniform sampler2DArray texAlbedoMaps[MAX_TEXTURE_LAYERS];
uniform sampler2DArray texDetailMaps[MAX_TEXTURE_LAYERS];

uniform float redDiffuseScale[MAX_TEXTURE_LAYERS];
uniform float redDetailScale[MAX_TEXTURE_LAYERS];
uniform float greenDiffuseScale[MAX_TEXTURE_LAYERS];
uniform float greenDetailScale[MAX_TEXTURE_LAYERS];
uniform float blueDiffuseScale[MAX_TEXTURE_LAYERS];
uniform float blueDetailScale[MAX_TEXTURE_LAYERS];
uniform float alphaDiffuseScale[MAX_TEXTURE_LAYERS];
uniform float alphaDetailScale[MAX_TEXTURE_LAYERS];

uniform sampler2D texWaterCaustics;
uniform sampler2D texUnderwaterAlbedo;
uniform sampler2D texUnderwaterDetail;
uniform float underwaterDiffuseScale;

vec4 getFinalColor1(in vec4 blendMap, in int index){
    return texture(texAlbedoMaps[index], vec3(_texCoord * redDiffuseScale[index], 0)) * blendMap.r;
}

vec4 getFinalColor2(in vec4 blendMap, in int index){
    vec4 redChannel   = texture(texAlbedoMaps[index], vec3(_texCoord * redDiffuseScale[index],   0)) * blendMap.r;
    vec4 greenChannel = texture(texAlbedoMaps[index], vec3(_texCoord * greenDiffuseScale[index], 1));

    return mix(redChannel , greenChannel, blendMap.g);
}

vec4 getFinalColor3(in vec4 blendMap, in int index){
    vec4 redChannel   = texture(texAlbedoMaps[index], vec3(_texCoord * redDiffuseScale[index],   0)) * blendMap.r;
    vec4 greenChannel = texture(texAlbedoMaps[index], vec3(_texCoord * greenDiffuseScale[index], 1));
    vec4 blueChannel  = texture(texAlbedoMaps[index], vec3(_texCoord * blueDiffuseScale[index],  2));
    
    greenChannel = mix(redChannel, greenChannel, blendMap.g);
    return mix(greenChannel, blueChannel, blendMap.b);
}

vec4 getFinalColor4(in vec4 blendMap, in int index){

    vec4 redChannel   = texture(texAlbedoMaps[index], vec3(_texCoord * redDiffuseScale[index],   0)) * blendMap.r;
    vec4 greenChannel = texture(texAlbedoMaps[index], vec3(_texCoord * greenDiffuseScale[index], 1));
    vec4 blueChannel  = texture(texAlbedoMaps[index], vec3(_texCoord * blueDiffuseScale[index],  2));
    vec4 alphaChannel = texture(texAlbedoMaps[index], vec3(_texCoord * alphaDiffuseScale[index], 3));

    greenChannel = mix(redChannel, greenChannel, blendMap.g);
    blueChannel  = mix(greenChannel, blueChannel, blendMap.b);

    return mix(blueChannel, alphaChannel, blendMap.a);
}

vec3 getFinalTBN1(in vec4 blendMap, in int index){
    return texture(texDetailMaps[index], vec3(_texCoord * redDetailScale[index], 0)).rgb * blendMap.r;
}

vec3 getFinalTBN2(in vec4 blendMap, in int index){
    vec4 redChannel   = texture(texDetailMaps[index], vec3(_texCoord * redDetailScale[index],   0)) * blendMap.r;
    vec4 greenChannel = texture(texDetailMaps[index], vec3(_texCoord * greenDetailScale[index], 1));

    return mix(redChannel, greenChannel, blendMap.g).rgb;
}

vec3 getFinalTBN3(in vec4 blendMap, in int index){
    vec4 redChannel   = texture(texDetailMaps[index], vec3(_texCoord * redDetailScale[index],   0)) * blendMap.r;
    vec4 greenChannel = texture(texDetailMaps[index], vec3(_texCoord * greenDetailScale[index], 1));
    vec4 blueChannel  = texture(texDetailMaps[index], vec3(_texCoord * blueDetailScale[index],  2));

    greenChannel = mix(redChannel, greenChannel, blendMap.g);

    return mix(greenChannel, blueChannel, blendMap.b).rgb;
}

vec3 getFinalTBN4(in vec4 blendMap, in int index){
    vec4 redChannel   = texture(texDetailMaps[index], vec3(_texCoord * redDetailScale[index],   0)) * blendMap.r;
    vec4 greenChannel = texture(texDetailMaps[index], vec3(_texCoord * greenDetailScale[index], 1));
    vec4 blueChannel  = texture(texDetailMaps[index], vec3(_texCoord * blueDetailScale[index],  2));
    vec4 alphaChannel = texture(texDetailMaps[index], vec3(_texCoord * alphaDetailScale[index], 3));

    greenChannel = mix(redChannel, greenChannel, blendMap.g);
    blueChannel = mix(greenChannel, blueChannel, blendMap.b);

    return mix(blueChannel, alphaChannel, blendMap.a).rgb;
}

void getColorAndTBNNormal(inout vec4 color, inout vec3 tbn){
    vec4 blendMap;

    for (int i = 0; i < MAX_TEXTURE_LAYERS; ++i) {
        blendMap = texture(texBlend[i], _texCoord);
#if CURRENT_TEXTURE_COUNT == 1 || CURRENT_TEXTURE_COUNT == 5 || CURRENT_TEXTURE_COUNT == 9
        color += getFinalColor1(blendMap, i);
        tbn += getFinalTBN1(blendMap, i);
#elif CURRENT_TEXTURE_COUNT == 2 || CURRENT_TEXTURE_COUNT == 6 || CURRENT_TEXTURE_COUNT == 10
        color += getFinalColor2(blendMap, i);
        tbn += getFinalTBN2(blendMap, i);
#elif CURRENT_TEXTURE_COUNT == 3 || CURRENT_TEXTURE_COUNT == 7 || CURRENT_TEXTURE_COUNT == 11
        color += getFinalColor3(blendMap, i);
        tbn += getFinalTBN3(blendMap, i);
#else //CURRENT_TEXTURE_COUNT == 4 || CURRENT_TEXTURE_COUNT == 8 || CURRENT_TEXTURE_COUNT == 12
        color += getFinalColor4(blendMap, i);
        tbn += getFinalTBN4(blendMap, i);
#endif
    }

    tbn = normalize(2.0 * (tbn / MAX_TEXTURE_LAYERS) - 1.0);
}

void getColorAndTBNUnderwater(inout vec4 color, inout vec3 tbn){
    vec2 coords = _texCoord * underwaterDiffuseScale;
    color = texture(texUnderwaterAlbedo, coords);
    tbn = normalize(2.0 * texture(texUnderwaterDetail, coords).rgb - 1.0);
}