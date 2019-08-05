#ifndef _TERRAIN_SPLATTING_FRAG_
#define _TERRAIN_SPLATTING_FRAG_

//#define TRI_PLANAR_BLEND

layout(binding = TEXTURE_SPLAT) uniform sampler2DArray texBlendMaps;

#if !defined(PRE_PASS)
layout(binding = TEXTURE_ALBEDO_TILE) uniform sampler2DArray texTileMaps;
#endif

layout(binding = TEXTURE_NORMAL_TILE) uniform sampler2DArray texNormalMaps;
layout(binding = TEXTURE_EXTRA_TILE) uniform sampler2DArray texExtraMaps;
layout(binding = TEXTURE_HELPER_TEXTURES)  uniform sampler2DArray helperTextures;

#include "texturing.frag"

struct TerrainData {
#if !defined(PRE_PASS)
    vec4  albedo;
#endif
    vec3  normal;
    vec2  uv;
#if !defined(PRE_PASS)
    float roughness;
    float ao;
#endif
};


const int tiling[] = {
    int(TEXTURE_TILE_SIZE * 1.0f),
    int(TEXTURE_TILE_SIZE * 0.25f),
    int(TEXTURE_TILE_SIZE * 0.0625f),
    int(TEXTURE_TILE_SIZE * 0.015625f),
    int(TEXTURE_TILE_SIZE * 0.00390625f),
    int(TEXTURE_TILE_SIZE * 0.0009765625f),
};


vec4 _getTexture(in sampler2DArray tex, in vec3 coords) {
    if (DETAIL_LEVEL > 2 && LoD == 0) {
        return vec4(textureNoTile(tex, helperTextures, 3, coords), 1.0f);
    }

    if (DETAIL_LEVEL > 1) {
        return vec4(textureNoTile(tex, coords), 1.0f);
    }

    return texture(tex, coords);
}

#if defined(PRE_PASS) || !defined(USE_DEFERRED_NORMALS)
#if defined(TRI_PLANAR_BLEND)
vec3 _getTriPlanarBlend(vec3 _wNorm) {
    // in wNorm is the world-space normal of the fragment
    vec3 blending = abs(_wNorm);
    blending = normalize(max(blending, 0.00001)); // Force weights to sum to 1.0
    float b = (blending.x + blending.y + blending.z);
    blending /= vec3(b, b, b);
    return blending;
}
#endif //TRI_PLANAR_BLEND
#endif //PRE_PASS || !USE_DEFERRED_NORMALS


vec4 _getUnderwaterAlbedo(in vec2 uv, in float waterDepth) {
    vec2 coords = uv * UNDERWATER_TILE_SCALE;

#if defined(LOW_QUALITY)
    return texture(helperTextures, vec3(coords, 1));
#else //LOW_QUALITY
    float time2 = float(dvd_time) * 0.0001f;
    vec4 scrollingUV = vec4(coords, coords + time2);
    scrollingUV.s -= time2;

    return mix((texture(helperTextures, vec3(scrollingUV.st, 0)) + texture(helperTextures, vec3(scrollingUV.pq, 0))) * 0.5f,
                texture(helperTextures, vec3(coords, 1)),
                waterDepth);
#endif //LOW_QUALITY
}

#if defined(USE_PARALLAX_OCCLUSION_MAPPING)
float getDisplacementValue(vec2 uv) {
    const int tileScale = tiling[LoD];

    vec2 scaledCoords = scaledTextureCoords(uv, tileScale);

    float ret = 0.0f;

    uint offset = 0;
    for (uint i = 0; i < MAX_TEXTURE_LAYERS; ++i) {
        const vec4 blendColour = texture(texBlendMaps, vec3(uv, i));
        const uint layerCount = CURRENT_LAYER_COUNT[i];
        for (uint j = 0; j < layerCount; ++j) {
            const uint slice = DISPLACEMENT_IDX[offset + j];
            if (slice < 255) {
                ret = mix(ret, _getTexture(texExtraMaps, vec3(scaledCoords, slice)).r, blendColour[j]);
            }
        }
        offset += CURRENT_LAYER_COUNT[i];
    }

    return ret;
}
#endif


vec3 blend(vec3 texture1, float d1, float a1, vec3 texture2, float d2, float a2)
{
    float depth = 0.2;
    float ma = max(d1 + a1, d2 + a2) - depth;

    float b1 = max(d1 + a1 - ma, 0);
    float b2 = max(d2 + a2 - ma, 0);

    return (texture1 * b1 + texture2 * b2) / (b1 + b2);
}

TerrainData BuildTerrainData(in vec2 waterDetails) {
    const int tileScale = tiling[LoD];

    TerrainData ret;
    ret.uv = getTexCoord();
#if !defined(PRE_PASS)
    ret.albedo = vec4(0.0f, 0.0f, 0.0f, 1.0f);
    ret.roughness = 1.0f;
    ret.ao = 0.0f;
    ret.normal = getNormal(ret.uv);
#else //PRE_PASS
#if defined(LOW_QUALITY)
    ret.normal = VAR._normalWV;
#else //LOW_QUALITY
    ret.normal = vec3(1.0f);
#endif //LOW_QUALITY
#endif //PRE_PASS

    // Gather all blend ammounts
    float blendAmount[TOTAL_LAYER_COUNT] = float[TOTAL_LAYER_COUNT](0.0f);
    uint offset = 0;
    for (uint i = 0; i < MAX_TEXTURE_LAYERS; ++i) {
        const vec4 blendColour = texture(texBlendMaps, vec3(ret.uv, i));
        const uint layerCount = CURRENT_LAYER_COUNT[i];
        for (uint j = 0; j < layerCount; ++j) {
            blendAmount[offset + j] = blendColour[j];
        }
        offset += layerCount;
    }

    vec2 scaledCoords = scaledTextureCoords(ret.uv, tileScale);
    if (DETAIL_LEVEL > 0 && LoD == 0) {
#if defined(USE_PARALLAX_MAPPING)
        float displacement = 0.0f;
        for (uint i = 0; i < TOTAL_LAYER_COUNT; ++i) {
            const uint dSlice = DISPLACEMENT_IDX[i];
            if (dSlice < 255) {
                displacement = mix(displacement, _getTexture(texExtraMaps, vec3(scaledCoords, dSlice)).r, blendAmount[i]);
            }
        }

        const vec3 viewDir = normalize(-VAR._vertexWV.xyz);
        scaledCoords = ParallaxMapping(scaledCoords, viewDir, displacement);
#elif defined(USE_PARALLAX_OCCLUSION_MAPPING)
        const vec3 viewDir = normalize(-VAR._vertexWV.xyz);
        scaledCoords = ParallaxOcclusionMapping(scaledCoords, viewDir);
#endif //USE_PARALLAX_OCCLUSION_MAPPING
    }
#if !defined(PRE_PASS)
#if !defined(LOW_QUALITY)
    for (uint i = 0; i < TOTAL_LAYER_COUNT && LoD == 0; ++i) {
        const float amnt = blendAmount[i];
        // ROUGHNESS
        const uint rSlice = ROUGHNESS_IDX[i];
        if (rSlice < 255) {
            ret.roughness = mix(ret.roughness, _getTexture(texExtraMaps, vec3(scaledCoords, rSlice)).r, amnt);
        }
        // AO
        const uint aoSlice = AO_IDX[i];
        if (LoD == 0) {
            ret.ao = mix(ret.ao, _getTexture(texExtraMaps, vec3(scaledCoords, aoSlice)).r, amnt);
        }
    }
#endif //LOW_QUALITY
    for (uint i = 0; i < TOTAL_LAYER_COUNT; ++i) {
        // ALBEDO
        const uint aSlice = ALBEDO_IDX[i];
        if (aSlice < 255) {
            ret.albedo = mix(ret.albedo, _getTexture(texTileMaps, vec3(scaledCoords, aSlice)), blendAmount[i]);
        }
    }
    ret.albedo = mix(ret.albedo, _getUnderwaterAlbedo(ret.uv, waterDetails.y), waterDetails.x);

#else //PRE_PASS

#if !defined(LOW_QUALITY)
    for (uint i = 0; i < TOTAL_LAYER_COUNT; ++i) {
        // NORMAL
        const uint nSlice = NORMAL_IDX[i];
        if (nSlice < 255) {
            ret.normal = mix(ret.normal, _getTexture(texNormalMaps, vec3(scaledCoords, nSlice)).rgb, blendAmount[i]);
        }
    }

    ret.normal = VAR._tbn * mix(2.0f * ret.normal - 1.0f,
                                2.0f * texture(helperTextures, vec3(ret.uv * UNDERWATER_TILE_SCALE, 2)).rgb - 1.0f,
                                waterDetails.x);
#endif //LOW_QUALITY

    ret.normal = normalize(ret.normal);
#endif //PRE_PASS

    return ret;
}

#endif //_TERRAIN_SPLATTING_FRAG_