#ifndef _TERRAIN_SPLATTING_FRAG_
#define _TERRAIN_SPLATTING_FRAG_

#define STAGE_SHADOW 0
#define STAGE_REFLECTION 1
#define STAGE_REFRACTION 2
#define STAGE_DISPLAY 3

layout(binding = TEXTURE_SPLAT) uniform sampler2DArray texBlendMaps;
layout(binding = TEXTURE_HELPER_TEXTURES) uniform sampler2DArray helperTextures;

#if defined(PRE_PASS)
layout(binding = TEXTURE_NORMAL_TILE) uniform sampler2DArray texNormalMaps;
#else
layout(binding = TEXTURE_ALBEDO_TILE) uniform sampler2DArray texTileMaps;
#endif

layout(binding = TEXTURE_EXTRA_TILE) uniform sampler2DArray texExtraMaps;

#include "texturing.frag"

#if DETAIL_LEVEL > 0 && (defined(USE_PARALLAX_OCCLUSION_MAPPING) || defined(USE_PARALLAX_MAPPING))
#define HAS_PARALLAX
#endif

struct TerrainData {
#if !defined(PRE_PASS)
    vec4  albedo; // a = roughness
#endif
    vec3  normal;
    vec2  uv;
};


const int tiling[] = {
    int(TEXTURE_TILE_SIZE * 1.0f), //LoD 0
    int(TEXTURE_TILE_SIZE * 1.0f), //LoD 1
    int(TEXTURE_TILE_SIZE * 0.25f),
    int(TEXTURE_TILE_SIZE * 0.0625f),
    int(TEXTURE_TILE_SIZE * 0.015625f),
    int(TEXTURE_TILE_SIZE * 0.00390625f),
    int(TEXTURE_TILE_SIZE * 0.0009765625f),
};


vec4 _getTexture(in sampler2DArray tex, in vec2 uv, in uint arrayIdx) {
    const vec3 texUV = vec3(uv, arrayIdx);
#if (DETAIL_LEVEL > 1)
    if (LoD == 0) {
#if (DETAIL_LEVEL > 2)
        return textureNoTile(tex, helperTextures, 3, texUV, 0.5f);
#else
        return textureNoTile(tex, texUV);
#endif
    }
#endif

    return texture(tex, texUV);
}

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

float getDisplacementValueFromCoords(vec2 sampleUV, in float[TOTAL_LAYER_COUNT] amnt) {
    float ret = 0.0f;
    for (uint i = 0; i < TOTAL_LAYER_COUNT; ++i) {
        ret = mix(ret, _getTexture(texExtraMaps, sampleUV, DISPLACEMENT_IDX[i]).r, amnt[i]);
    }
    return ret;
}

float getDisplacementValue(vec2 sampleUV) {
    vec2 blendUV = unscaledTextureCoords(sampleUV, tiling[LoD]);

    INSERT_BLEND_AMNT_ARRAY;
    getBlendFactor(blendUV, blendAmount);

    return getDisplacementValueFromCoords(sampleUV, blendAmount);
}
#endif

vec2 _getScaledCoords(in vec2 uv, in float[TOTAL_LAYER_COUNT] amnt) {
    const vec2 scaledCoords = scaledTextureCoords(uv, tiling[LoD]);
#if defined(HAS_PARALLAX)
    if (LoD == 0) {
        const vec3 viewDir = normalize(-VAR._vertexWV.xyz);
        float currentHeight = getDisplacementValueFromCoords(scaledCoords, amnt);
#if defined(USE_PARALLAX_MAPPING)
        return ParallaxOffset(scaledCoords, viewDir, currentHeight);
#else
        return ParallaxOcclusionMapping(scaledCoords, viewDir, currentHeight);
#endif //USE_PARALLAX_OCCLUSION_MAPPING
    }
#endif //HAS_PARALLAX
    return scaledCoords;
}

#if defined(PRE_PASS)
vec3 _getTerrainNormal(in vec2 uv) {
    INSERT_BLEND_AMNT_ARRAY;
    getBlendFactor(uv, blendAmount);
    const vec2 scaledUV = _getScaledCoords(uv, blendAmount);

    vec3 normal = vec3(0.0f);
    for (uint i = 0; i < TOTAL_LAYER_COUNT; ++i) {
        normal = mix(normal, _getTexture(texNormalMaps, scaledUV, NORMAL_IDX[i]).rgb, blendAmount[i]);
    }

    return normal;
}

vec3 getMixedNormal(in vec2 uv, in float waterDepth) {
    return VAR._tbn * (2.0f * mix(texture(helperTextures, vec3(uv * UNDERWATER_TILE_SCALE, 2)).rgb,
                                  _getTerrainNormal(uv),
                                   saturate(waterDepth)) - 1.0f);
}
#else //PRE_PASS

vec4 _getUnderwaterAlbedo(in vec2 uv, in float waterDepth) {
#if defined(LOW_QUALITY)
    return vec4(texture(helperTextures, vec3(uv, 1)).rgb, 0.3f);
#else //LOW_QUALITY
    float time2 = float(dvd_time) * 0.0001f;
    vec4 scrollingUV = vec4(uv, uv + time2);
    scrollingUV.s -= time2;

    return vec4(mix((texture(helperTextures, vec3(scrollingUV.st, 0)).rgb + texture(helperTextures, vec3(scrollingUV.pq, 0)).rgb) * 0.5f,
                    texture(helperTextures, vec3(uv, 1)).rgb,
                    waterDepth),
                0.3f);
#endif //LOW_QUALITY
}

vec4 _getTerrainAlbedo(in vec2 uv) {
    INSERT_BLEND_AMNT_ARRAY;
    getBlendFactor(uv, blendAmount);
    const vec2 scaledUV = _getScaledCoords(uv, blendAmount);

    vec4 ret = vec4(0.0f);
    for (uint i = 0; i < TOTAL_LAYER_COUNT; ++i) {
        // Albedo & Roughness
        ret = mix(ret, _getTexture(texTileMaps, scaledUV, ALBEDO_IDX[i]), blendAmount[i]);
        // ToDo: AO
    }

    return ret;
}
#endif // PRE_PASS

void BuildTerrainData(in vec2 waterDetails, out TerrainData data) {
    data.uv = TexCoords;
#   if defined(LOW_QUALITY)
    data.normal = VAR._normalWV;
#   endif

#if defined(PRE_PASS)
#   if !defined(LOW_QUALITY)
    data.normal = getMixedNormal(data.uv, 1.0f - waterDetails.x);
#   endif //LOW_QUALITY
#else // PRE_PASS
#   if !defined(LOW_QUALITY)
    data.normal = getNormal(data.uv);
#   endif //LOW_QUALITY
    data.albedo =  mix(_getUnderwaterAlbedo(data.uv * UNDERWATER_TILE_SCALE, waterDetails.y),
                       _getTerrainAlbedo(data.uv),
                       1.0f - waterDetails.x);
#endif //PRE_PASS
}


#endif //_TERRAIN_SPLATTING_FRAG_