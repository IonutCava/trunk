#ifndef _TERRAIN_SPLATTING_FRAG_
#define _TERRAIN_SPLATTING_FRAG_

layout(binding = TEXTURE_SPLAT) uniform sampler2DArray texBlendMaps;
layout(binding = TEXTURE_HELPER_TEXTURES) uniform sampler2DArray helperTextures;
layout(binding = TEXTURE_EXTRA_TILE) uniform sampler2DArray texExtraMaps;

#if defined(PRE_PASS)
layout(binding = TEXTURE_NORMAL_TILE) uniform sampler2DArray texNormalMaps;
#else
layout(binding = TEXTURE_ALBEDO_TILE) uniform sampler2DArray texTileMaps;
#endif

#include "texturing.frag"

const float tiling[] = {
    1.5f, //LoD 0
    1.0f, //LoD 1
    0.75f,
    0.5f,
    0.25f,
    0.1625f,
    0.083125f,
};

#if defined(LOW_QUALITY)
#define sampleTexture texture
#else
vec4 sampleTexture(in sampler2DArray tex, in vec3 texUV) {
#if defined(REDUCE_TEXTURE_TILE_ARTIFACT) 

#if !defined(REDUCE_TEXTURE_TILE_ARTIFACT_ALL_LODS)
    if (LoD < 2) 
#endif //REDUCE_TEXTURE_TILE_ARTIFACT_ALL_LODS

    {
#if defined(HIGH_QUALITY_TILE_ARTIFACT_REDUCTION)
        return textureNoTile(tex, helperTextures, 3, texUV, 0.5f);
#else  //HIGH_QUALITY_TILE_ARTIFACT_REDUCTION
        return textureNoTile(tex, texUV);
#endif //HIGH_QUALITY_TILE_ARTIFACT_REDUCTION
    }

#endif //REDUCE_TEXTURE_TILE_ARTIFACT

    return texture(tex, texUV);
}
#endif //LOW_QUALITY

float[TOTAL_LAYER_COUNT] getBlendFactor(in vec2 uv) {
    INSERT_BLEND_AMNT_ARRAY

    uint offset = 0;
    for (uint i = 0; i < MAX_TEXTURE_LAYERS; ++i) {
        const vec4 blendColour = texture(texBlendMaps, vec3(uv, i));
        const uint layerCount = CURRENT_LAYER_COUNT[i];
        for (uint j = 0; j < layerCount; ++j) {
            blendAmount[offset + j] = blendColour[j];
        }
        offset += layerCount;
    }

    return blendAmount;
}

vec2 scaledTextureCoords(in vec2 uv) {
    return scaledTextureCoords(uv, TEXTURE_TILE_SIZE * tiling[LoD]);
}

#if defined(HAS_PARALLAX)
float getDisplacementValueFromCoords(in vec2 sampleUV, in float[TOTAL_LAYER_COUNT] amnt) {
    float ret = 0.0f;
    for (uint i = 0; i < TOTAL_LAYER_COUNT; ++i) {
        ret = mix(ret, sampleTexture(texExtraMaps, vec3(sampleUV, DISPLACEMENT_IDX[i])).r, amnt[i]);
    }

    return 1.0f - ret;
}

vec2 ParallaxOcclusionMapping(vec2 sampleUV, vec3 viewDirTBN, float currentDepthMapValue, in float[TOTAL_LAYER_COUNT] amnt) {
    // number of depth layers
    const float minLayers = 8.0;
    const float maxLayers = 32.0;
    float numLayers = mix(maxLayers, minLayers, abs(dot(vec3(0.0, 0.0, 1.0), viewDirTBN)));
    // calculate the size of each layer
    float layerDepth = 1.0 / numLayers;
    // depth of current layer
    float currentLayerDepth = 0.0;
    // the amount to shift the texture coordinates per layer (from vector P)
    vec2 P = viewDirTBN.xy * dvd_parallaxFactor;
    vec2 deltaTexCoords = P / numLayers;

    // get initial values
    vec2  currentTexCoords = sampleUV;
    while (currentLayerDepth < currentDepthMapValue) {
        // shift texture coordinates along direction of P
        currentTexCoords -= deltaTexCoords;
        // get depthmap value at current texture coordinates
        currentDepthMapValue = getDisplacementValueFromCoords(scaledTextureCoords(currentTexCoords), amnt);
        // get depth of next layer
        currentLayerDepth += layerDepth;
    }

    const vec2 prevTexCoords = currentTexCoords + deltaTexCoords;

    // get depth after and before collision for linear interpolation
    const float afterDepth = currentDepthMapValue - currentLayerDepth;
    const float beforeDepth = getDisplacementValueFromCoords(scaledTextureCoords(prevTexCoords), amnt) - currentLayerDepth + layerDepth;

    // interpolation of texture coordinates
    const float weight = afterDepth / (afterDepth - beforeDepth);
    return prevTexCoords * weight + currentTexCoords * (1.0f - weight);
}

#endif //HAS_PARALLAX
vec2 getScaledCoords(in float[TOTAL_LAYER_COUNT] amnt) {
    vec2 scaledCoords = scaledTextureCoords(TexCoords);

#if defined(HAS_PARALLAX)
    /*const vec3 viewDirTBN = normalize(_tangentViewPos - _tangentFragPos);
    if (LoD < 2 && dvd_bumpMethod != BUMP_NONE) {
        float currentHeight = getDisplacementValueFromCoords(scaledCoords, amnt);
        if (dvd_bumpMethod == BUMP_PARALLAX) {
            return ParallaxOffset(scaledCoords, viewDirTBN, currentHeight);
        } else if (dvd_bumpMethod == BUMP_PARALLAX_OCCLUSION) {
            return ParallaxOcclusionMapping(TexCoords, viewDirTBN, currentHeight, amnt);
        }
    }*/
#endif //HAS_PARALLAX

    return scaledCoords;
}

#if defined(PRE_PASS)

vec3 getTerrainNormal() {
    float blendAmount[TOTAL_LAYER_COUNT] = getBlendFactor(TexCoords);

    const vec2 scaledUV = getScaledCoords(blendAmount);
    vec3 normal = vec3(0.0f);
    for (uint i = 0; i < TOTAL_LAYER_COUNT; ++i) {
        normal = mix(normal, sampleTexture(texNormalMaps, vec3(scaledUV, NORMAL_IDX[i])).rgb, blendAmount[i]);
    }

    return normal;
}

vec3 getMixedNormal(in float waterDepth) {
    const vec3 ret = mix(texture(helperTextures, vec3(TexCoords * UNDERWATER_TILE_SCALE, 2)).rgb, 
                         getTerrainNormal(),
                         saturate(waterDepth));

    return VAR._tbn * (2.0f * ret - 1.0f);
}

#else //PRE_PASS

vec4 getUnderwaterAlbedo(in float waterDepth) {
    const vec2 scaledUV = TexCoords * UNDERWATER_TILE_SCALE;
#if defined(LOW_QUALITY)
    return vec4(texture(helperTextures, vec3(scaledUV, 1)).rgb, 0.3f);
#else //LOW_QUALITY
    float time2 = float(dvd_time) * 0.0001f;
    vec4 scrollingUV = vec4(scaledUV, scaledUV + time2);
    scrollingUV.s -= time2;

    return vec4(mix((texture(helperTextures, vec3(scrollingUV.st, 0)).rgb + texture(helperTextures, vec3(scrollingUV.pq, 0)).rgb) * 0.5f,
                     texture(helperTextures, vec3(scaledUV, 1)).rgb,
                     waterDepth),
                0.3f);
#endif //LOW_QUALITY
}

vec4 getTerrainAlbedo() {
    float blendAmount[TOTAL_LAYER_COUNT] = getBlendFactor(TexCoords);
    const vec2 scaledUV = getScaledCoords(blendAmount);

    vec4 ret = vec4(0.0f);
    for (uint i = 0; i < TOTAL_LAYER_COUNT; ++i) {
        // Albedo & Roughness
        ret = mix(ret, sampleTexture(texTileMaps, vec3(scaledUV, ALBEDO_IDX[i])), blendAmount[i]);
        // ToDo: AO
    }
    return ret;
}

void BuildTerrainData(in vec2 waterDetails, out vec4 albedo, out vec3 normalWV) {
#if defined(LOW_QUALITY)
    normalWV = VAR._normalWV;
#else //LOW_QUALITY
    normalWV = getNormal(TexCoords);
#endif //LOW_QUALITY

    albedo = mix(getTerrainAlbedo(),
                  getUnderwaterAlbedo(waterDetails.y),
                  saturate(waterDetails.x));
    
}
#endif // PRE_PASS

#endif //_TERRAIN_SPLATTING_FRAG_