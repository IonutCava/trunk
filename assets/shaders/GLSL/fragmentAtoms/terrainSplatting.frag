#ifndef _TERRAIN_SPLATTING_FRAG_
#define _TERRAIN_SPLATTING_FRAG_

#define SAMPLE_NO_TILE_ARRAYS
#include "texturing.frag"
#include "waterData.cmn"

#if defined(LOW_QUALITY) || !defined(REDUCE_TEXTURE_TILE_ARTIFACT)
#define SampleTextureNoTile texture
#else //LOW_QUALITY || !REDUCE_TEXTURE_TILE_ARTIFACT
vec4 SampleTextureNoTile(in sampler2DArray tex, in vec3 texUV) {
#if defined(HIGH_QUALITY_TILE_ARTIFACT_REDUCTION)

#if !defined(REDUCE_TEXTURE_TILE_ARTIFACT_ALL_LODS)
    return dvd_LoD < 2 ? textureNoTile(tex, texOMR, 3, texUV, 0.5f) : texture(tex, texUV);
#else 
    return textureNoTile(tex, texOMR, 3, texUV, 0.5f);
#endif //REDUCE_TEXTURE_TILE_ARTIFACT_ALL_LODS

#else  //HIGH_QUALITY_TILE_ARTIFACT_REDUCTION

#if !defined(REDUCE_TEXTURE_TILE_ARTIFACT_ALL_LODS)
    return dvd_LoD < 2 ? textureNoTile(tex, texUV) : texture(tex, texUV);
#else 
    return textureNoTile(tex, texUV);
#endif //REDUCE_TEXTURE_TILE_ARTIFACT_ALL_LODS

#endif //HIGH_QUALITY_TILE_ARTIFACT_REDUCTION
}

#endif ///LOW_QUALITY || !REDUCE_TEXTURE_TILE_ARTIFACT

float[TOTAL_LAYER_COUNT] getBlendFactor(in vec2 uv) {
    INSERT_BLEND_AMNT_ARRAY

    uint offset = 0;
    for (uint i = 0; i < MAX_TEXTURE_LAYERS; ++i) {
        const vec4 blendColour = texture(texOpacityMap, vec3(uv, i));
        for (uint j = 0; j < CURRENT_LAYER_COUNT[i]; ++j) {
            blendAmount[offset + j] = blendColour[j];
        }
        offset += CURRENT_LAYER_COUNT[i];
    }

    return blendAmount;
}

const float tiling[] = {
#if defined(REDUCE_TEXTURE_TILE_ARTIFACT)
    1.5f, //LoD 0
    1.5f, //LoD 1
    1.0f,
#else //REDUCE_TEXTURE_TILE_ARTIFACT
    1.5f, //LoD 0
    1.0f, //LoD 1
    0.75f,
#endif //REDUCE_TEXTURE_TILE_ARTIFACT
    0.5f,
    0.25f,
    0.1625f,
    0.083125f,
};

#define scaledTextureCoords(UV) scaledTextureCoords(UV, TEXTURE_TILE_SIZE * tiling[dvd_LoD])

mat3 getTBNWV() { return VAR._tbnWV; }
vec3 getTBNViewDir() { return VAR._tbnViewDir; }

#if defined(HAS_PARALLAX)
float getDisplacementValueFromCoords(in vec2 sampleUV, in float[TOTAL_LAYER_COUNT] amnt) {
    float ret = 0.0f;
    for (uint i = 0; i < TOTAL_LAYER_COUNT; ++i) {
        ret = mix(ret, SampleTextureNoTile(texProjected, vec3(sampleUV, DISPLACEMENT_IDX[i])).r, amnt[i]);
    }

    return 1.0f - ret;
}

vec2 ParallaxOcclusionMapping(vec2 sampleUV, float currentDepthMapValue, in float[TOTAL_LAYER_COUNT] amnt) {
    // number of depth layers
    const float minLayers = 8.0;
    const float maxLayers = 32.0;
    float numLayers = mix(maxLayers, minLayers, abs(dot(vec3(0.0, 0.0, 1.0), dvd_TBNViewDir)));
    // calculate the size of each layer
    float layerDepth = 1.0 / numLayers;
    // depth of current layer
    float currentLayerDepth = 0.0;
    // the amount to shift the texture coordinates per layer (from vector P)
    vec2 P = dvd_TBNViewDir.xy * dvd_parallaxFactor(MATERIAL_IDX);
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
#if !defined(LOW_QUALITY) && defined(HAS_PARALLAX)
vec2 getScaledCoords(in vec2 uv, in float[TOTAL_LAYER_COUNT] amnt) {
    const vec2 scaledCoords = scaledTextureCoords(uv);

    const uint bumpMethod = dvd_LoD < 2 ? BUMP_NONE : dvd_bumpMethod(MATERIAL_IDX);

    switch(bumpMethod) {
        case BUMP_NONE: break;
        case BUMP_PARALLAX:
        {
            const float currentHeight = getDisplacementValueFromCoords(scaledCoords, amnt);
            return ParallaxOffset(scaledCoords, currentHeight);
        }
        case BUMP_PARALLAX_OCCLUSION:
        {
            const float currentHeight = getDisplacementValueFromCoords(scaledCoords, amnt);
            return ParallaxOcclusionMapping(uv, currentHeight, amnt);
        }
    }

    return scaledCoords;
}
#else //!LOW_QUALITY && HAS_PARALLAX
#define getScaledCoords(UV, AMNT) scaledTextureCoords(UV)
#endif //!LOW_QUALITY && HAS_PARALLAX

#if defined(PRE_PASS)
vec3 getTerrainNormal(in vec2 uv) {
    float blendAmount[TOTAL_LAYER_COUNT] = getBlendFactor(uv);
    const vec2 scaledUV = getScaledCoords(uv, blendAmount);
    vec3 normal = vec3(0.0f);
    for (uint i = 0; i < TOTAL_LAYER_COUNT; ++i) {
        normal = mix(normal, SampleTextureNoTile(texDiffuse1, vec3(scaledUV, NORMAL_IDX[i])).rgb, blendAmount[i]);
    }

    return normal;
}

#if defined(PRE_PASS) && !defined(HAS_PRE_PASS_DATA)
#define getMixedNormalWV(UV) vec3(0.0f)
#else//PRE_PASS && !HAS_PRE_PASS_DATA
#if defined(LOW_QUALITY)
#define getMixedNormalWV(UV) VAR._normalWV
#else //LOW_QUALITY
vec3 getMixedNormalWV(in vec2 uv) {
    const float waterDist = GetWaterDetails(VAR._vertexW.xyz, TERRAIN_HEIGHT_OFFSET).x;
    const vec3 normalTBN = mix(texture(texOMR, vec3(uv * UNDERWATER_TILE_SCALE, 2)).rgb,
                               getTerrainNormal(uv),
                               saturate(1.0f - waterDist));

    return VAR._tbnWV * (2.0f * normalTBN - 1.0f);
}
#endif //LOW_QUALITY
#endif //PRE_PASS && !HAS_PRE_PASS_DATA

#else //PRE_PASS

vec3 getUnderWaterAnimatedAlbedo(in vec2 uv) {
    const float time2 = MSToSeconds(dvd_time) * 0.1f;
    const vec4 uvNormal = vec4(uv + time2.xx, uv + vec2(-time2, time2));

    return (texture(texOMR, vec3(uvNormal.xy, 0)).rgb + texture(texOMR, vec3(uvNormal.zw, 0)).rgb) * 0.5f;
}

vec4 getUnderwaterAlbedo(in vec2 uv, in float waterDepth) {
    const vec3 albedo = mix(getUnderWaterAnimatedAlbedo(uv), texture(texOMR, vec3(uv,  1)).rgb, waterDepth);
    return vec4(albedo, 0.3f);
}

vec4 getTerrainAlbedo(in vec2 uv) {
    const float blendAmount[TOTAL_LAYER_COUNT] = getBlendFactor(uv);
    const vec2 scaledUV = getScaledCoords(uv, blendAmount);

    vec4 ret = vec4(0.0f);
    for (uint i = 0; i < TOTAL_LAYER_COUNT; ++i) {
        // Albedo & Roughness
        ret = mix(ret, SampleTextureNoTile(texDiffuse0, vec3(scaledUV, ALBEDO_IDX[i])), blendAmount[i]);
        // ToDo: AO
    }
    return ret;
}

void BuildTerrainData(in vec2 uv, out vec4 albedo, out vec3 normalWV) {
#if defined(LOW_QUALITY)
    normalWV = VAR._normalWV;
#else //LOW_QUALITY
    normalWV = getNormalWV(uv);
#endif //LOW_QUALITY

    const vec3 vertW = vec3(uv.x * TERRAIN_WIDTH - (TERRAIN_WIDTH * 0.5f),
                            VAR._vertexW.y,
                            uv.y * TERRAIN_LENGTH - (TERRAIN_LENGTH * 0.5f));

    const vec2 waterData = GetWaterDetails(vertW, TERRAIN_HEIGHT_OFFSET);
    albedo = mix(getTerrainAlbedo(uv),
                 getUnderwaterAlbedo(uv * UNDERWATER_TILE_SCALE, waterData.y),
                 saturate(waterData.x));
    
}
#endif // PRE_PASS

#endif //_TERRAIN_SPLATTING_FRAG_