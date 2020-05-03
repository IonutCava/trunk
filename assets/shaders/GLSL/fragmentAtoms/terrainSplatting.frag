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

const int tiling[] = {
    int(TEXTURE_TILE_SIZE * 1.0f), //LoD 0
    int(TEXTURE_TILE_SIZE * 1.0f), //LoD 1
    int(TEXTURE_TILE_SIZE * 0.25f),
    int(TEXTURE_TILE_SIZE * 0.0625f),
    int(TEXTURE_TILE_SIZE * 0.015625f),
    int(TEXTURE_TILE_SIZE * 0.00390625f),
    int(TEXTURE_TILE_SIZE * 0.0009765625f),
};


#if defined(LOW_QUALITY)
#define sampleTexture texture;
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

void getBlendFactor(in vec2 uv, inout float blendAmount[TOTAL_LAYER_COUNT]) {
    uint offset = 0;
    for (uint i = 0; i < MAX_TEXTURE_LAYERS; ++i) {
        const vec4 blendColour = texture(texBlendMaps, vec3(uv, i));
        const uint layerCount = CURRENT_LAYER_COUNT[i];
        for (uint j = 0; j < layerCount; ++j) {
            blendAmount[offset + j] = blendColour[j];
        }
        offset += layerCount;
    }
}

#if defined(HAS_PARALLAX)
float getDisplacementValueFromCoords(in vec2 sampleUV, in float[TOTAL_LAYER_COUNT] amnt) {
    float ret = 0.0f;
    for (uint i = 0; i < TOTAL_LAYER_COUNT; ++i) {
        ret = mix(ret, sampleTexture(texExtraMaps, vec3(sampleUV, DISPLACEMENT_IDX[i])).r, amnt[i]);
    }
    return ret;
}

float getDisplacementValue(in vec2 sampleUV) {
    const vec2 blendUV = unscaledTextureCoords(sampleUV, tiling[LoD]);

    INSERT_BLEND_AMNT_ARRAY;
    getBlendFactor(blendUV, blendAmount);

    return getDisplacementValueFromCoords(sampleUV, blendAmount);
}
#endif //HAS_PARALLAX

vec2 getScaledCoords(in float[TOTAL_LAYER_COUNT] amnt) {
    const vec2 scaledCoords = scaledTextureCoords(TexCoords, tiling[LoD]);

#if defined(HAS_PARALLAX)
    if (LoD < 2 && dvd_bumpMethod != BUMP_NONE) {
        float currentHeight = getDisplacementValueFromCoords(scaledCoords, amnt);
        if (dvd_bumpMethod == BUMP_PARALLAX) {
            return ParallaxOffset(scaledCoords, VAR._viewDirectionWV, currentHeight);
        } else if (dvd_bumpMethod == BUMP_PARALLAX_OCCLUSION) {
            return ParallaxOcclusionMapping(scaledCoords, VAR._viewDirectionWV, currentHeight);
        }
    }
#endif //HAS_PARALLAX

    return scaledCoords;
}

#if defined(PRE_PASS)

vec3 getTerrainNormal() {
    INSERT_BLEND_AMNT_ARRAY;
    getBlendFactor(TexCoords, blendAmount);
    const vec2 scaledUV = getScaledCoords(blendAmount);

    vec3 normal = vec3(0.0f);
    for (uint i = 0; i < TOTAL_LAYER_COUNT; ++i) {
        normal = mix(normal, sampleTexture(texNormalMaps, vec3(scaledUV, NORMAL_IDX[i])).rgb, blendAmount[i]);
    }

    return normal;
}

vec3 getMixedNormal(in float waterDepth) {
    vec3 ret = mix(texture(helperTextures, vec3(TexCoords * UNDERWATER_TILE_SCALE, 2)).rgb,
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
    INSERT_BLEND_AMNT_ARRAY;
    getBlendFactor(TexCoords, blendAmount);
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

    albedo =  mix(getUnderwaterAlbedo(waterDetails.y),
                  getTerrainAlbedo(),
                  1.0f - waterDetails.x);
}
#endif // PRE_PASS

#endif //_TERRAIN_SPLATTING_FRAG_