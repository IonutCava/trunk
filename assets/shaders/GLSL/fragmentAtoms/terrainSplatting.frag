#ifndef _TERRAIN_SPLATTING_FRAG_
#define _TERRAIN_SPLATTING_FRAG_

#if !defined(PRE_PASS) || defined(HAS_PRE_PASS_DATA)
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

#endif //LOW_QUALITY || !REDUCE_TEXTURE_TILE_ARTIFACT

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

#define scaledTextureCoords(UV) scaledTextureCoords(UV, TEXTURE_TILE_SIZE)

mat3 cotangentFrame(in vec3 N, in vec3 p, in vec2 uv) {
    // get edge vectors of the pixel triangle
    const vec3 dp1 = dFdx(p);
    const vec3 dp2 = dFdy(p);
    const vec2 duv1 = dFdx(uv);
    const vec2 duv2 = dFdy(uv);

    // solve the linear system
    const vec3 dp2perp = cross(dp2, N);
    const vec3 dp1perp = cross(N, dp1);
    const vec3 T = dp2perp * duv1.x + dp1perp * duv2.x;
    const vec3 B = dp2perp * duv1.y + dp1perp * duv2.y;

    // construct a scale-invariant frame 
    const float invmax = inversesqrt(max(dot(T, T), dot(B, B)));
    return mat3(T * invmax,
                B * invmax,
                N);
}

mat3 computeTBN() {
    const vec3 V = normalize(dvd_cameraPosition.xyz - VAR._vertexW.xyz);
    return cotangentFrame(VAR._normalWV, -V, VAR._texCoord);
}

mat3 getTBNWV() {
#if 0
    return mat3(dvd_ViewMatrix) * computeTBN();
#else
    // We need to compute tangent and bitengent vectors with 
    // as cotangent_frame's results do not apply for what we need them to do
    const vec3 N = normalize(VAR._normalWV);
    const vec3 T1 = cross(N, vec3(0.f, 0.f, 1.f));
    const vec3 T2 = cross(N, vec3(0.f, 1.f, 0.f));
    const vec3 T = normalize(length(T1) > length(T2) ? T1 : T2);
    const vec3 B = normalize(-cross(N, T));
    // Orthogonal matrix(each axis is a perpendicular unit vector)
    // The transpose of an orthogonal matrix equals its inverse
    return mat3(dvd_ViewMatrix) * mat3(T, B, N);
#endif
}

#if defined(HAS_PARALLAX)
float getDisplacementValueFromCoords(in vec2 sampleUV, in float[TOTAL_LAYER_COUNT] amnt) {
    float ret = 0.0f;
    for (uint i = 0; i < TOTAL_LAYER_COUNT; ++i) {
        ret = max(ret, SampleTextureNoTile(texProjected, vec3(sampleUV * CURRENT_TILE_FACTORS[i], DISPLACEMENT_IDX[i])).r * amnt[i]);
    }
    // Transform the height to displacement (easier to fake depth than height on flat surfaces)
    return 1.f - ret;
}

vec2 ParallaxOcclusionMapping(in vec2 scaledCoords, in vec3 viewDirT, float currentDepthMapValue, float heightFactor, in float[TOTAL_LAYER_COUNT] amnt) {
    // number of depth layers
    const float minLayers = 8.f;
    const float maxLayers = 32.f;
    float numLayers = mix(maxLayers, minLayers, abs(dot(vec3(0.f, 0.f, 1.f), viewDirT)));
    // calculate the size of each layer
    float layerDepth = 1.f / numLayers;
    // depth of current layer
    float currentLayerDepth = 0.f;
    // the amount to shift the texture coordinates per layer (from vector P)
    vec2 P = viewDirT.xy * heightFactor;
    vec2 deltaTexCoords = P / numLayers;

    // get initial values
    vec2 currentTexCoords = scaledCoords;
    while (currentLayerDepth < currentDepthMapValue) {
        // shift texture coordinates along direction of P
        currentTexCoords -= deltaTexCoords;
        // get depthmap value at current texture coordinates
        currentDepthMapValue = getDisplacementValueFromCoords(currentTexCoords, amnt);
        // get depth of next layer
        currentLayerDepth += layerDepth;
    }

    const vec2 prevTexCoords = currentTexCoords + deltaTexCoords;

    // get depth after and before collision for linear interpolation
    const float afterDepth = currentDepthMapValue - currentLayerDepth;
    const float beforeDepth = getDisplacementValueFromCoords(prevTexCoords, amnt) - currentLayerDepth + layerDepth;

    // interpolation of texture coordinates
    const float weight = afterDepth / (afterDepth - beforeDepth);
    return prevTexCoords * weight + currentTexCoords * (1.f - weight);
}

#endif //HAS_PARALLAX
#if !defined(LOW_QUALITY) && defined(HAS_PARALLAX)
vec2 getScaledCoords(vec2 uv, in float[TOTAL_LAYER_COUNT] amnt) {
    const vec2 scaledCoords = scaledTextureCoords(uv);

    const uint bumpMethod = dvd_LoD >= 2 ? BUMP_NONE : dvd_bumpMethod(MATERIAL_IDX);
    if (bumpMethod != BUMP_NONE) {
        const float heightFactor = dvd_parallaxFactor(MATERIAL_IDX);
#if 1
        // We need to compute tangent and bitengent vectors with 
        // as cotangent_frame's results do not apply for what we need them to do
        const vec3 N = normalize(VAR._normalWV);
        const vec3 T1 = cross(N, vec3(0.f, 0.f, 1.f));
        const vec3 T2 = cross(N, vec3(0.f, 1.f, 0.f));
        const vec3 T = normalize(length(T1) > length(T2) ? T1 : T2);
        const vec3 B = normalize(-cross(N, T));
        // Orthogonal matrix(each axis is a perpendicular unit vector)
        // The transpose of an orthogonal matrix equals its inverse
        const mat3 invTBN = transpose(mat3(T, B, N));
#else
        const mat3 invTBN = transpose(computeTBN());
#endif
        const vec3 viewDirT = invTBN * normalize(dvd_cameraPosition.xyz - VAR._vertexW.xyz);

        const float currentHeight = getDisplacementValueFromCoords(scaledCoords, amnt);
        switch (bumpMethod) {
            case BUMP_PARALLAX: {
                //ref: https://learnopengl.com/Advanced-Lighting/Parallax-Mapping
                const vec2 p = viewDirT.xy / viewDirT.z * currentHeight * heightFactor;
                const vec2 texCoords = uv - p;
                const vec2 clippedTexCoord = vec2(texCoords.x - floor(texCoords.x),
                                                  texCoords.y - floor(texCoords.y));
                //return scaledTextureCoords(clippedTexCoord);
                    
            } break;
            case BUMP_PARALLAX_OCCLUSION:
            {
                //return  ParallaxOcclusionMapping(correctedUV, viewDirT, currentHeight, heightFactor, amnt);
            } break;
        }
    }

    return scaledCoords;
}
#else //!LOW_QUALITY && HAS_PARALLAX
#define getScaledCoords(UV, AMNT) scaledTextureCoords(UV)
#endif //!LOW_QUALITY && HAS_PARALLAX

#if defined(PRE_PASS)
vec3 getTerrainNormal() {
    float blendAmount[TOTAL_LAYER_COUNT] = getBlendFactor(VAR._texCoord);
    const vec2 uv = getScaledCoords(VAR._texCoord, blendAmount);

    vec3 normal = vec3(0.0f);
    for (uint i = 0; i < TOTAL_LAYER_COUNT; ++i) {
        normal = mix(normal, SampleTextureNoTile(texDiffuse1, vec3(uv * CURRENT_TILE_FACTORS[i], NORMAL_IDX[i])).rgb, blendAmount[i]);
    }

    return normal;
}

#else //PRE_PASS

vec4 getTerrainAlbedo() {
    const float blendAmount[TOTAL_LAYER_COUNT] = getBlendFactor(VAR._texCoord);
    const vec2 uv = getScaledCoords(VAR._texCoord, blendAmount);

    vec4 albedo = vec4(0.0f);
    for (uint i = 0; i < TOTAL_LAYER_COUNT; ++i) {
        // Albedo & Roughness
        albedo = mix(albedo, SampleTextureNoTile(texDiffuse0, vec3(uv * CURRENT_TILE_FACTORS[i], ALBEDO_IDX[i])), blendAmount[i]);
    }

    return albedo;
}

vec4 getUnderwaterAlbedo(in vec2 uv, in float waterDepth) {
    const float time2 = MSToSeconds(dvd_time) * 0.1f;
    const vec4 uvNormal = vec4(uv + time2.xx, uv + vec2(-time2, time2));

    return vec4(mix(0.5f * (texture(texOMR, vec3(uvNormal.xy, 0)).rgb + texture(texOMR, vec3(uvNormal.zw, 0)).rgb),
                    texture(texOMR, vec3(uv, 1)).rgb,
                    waterDepth),
                0.3f);
}

#endif // PRE_PASS
#endif //!PRE_PASS || HAS_PRE_PASS_DATA


vec4 BuildTerrainData(out vec3 normalWV) {
#if defined(LOW_QUALITY)
    // VAR._normalWV is in world-space
    normalWV = mat3(dvd_ViewMatrix) * VAR._normalWV;
#endif //LOW_QUALITY

#if defined(PRE_PASS)

#if defined(HAS_PRE_PASS_DATA) && !defined(LOW_QUALITY)
    const vec2 waterData = GetWaterDetails(VAR._vertexW.xyz, TERRAIN_HEIGHT_OFFSET);
    const vec3 normalMap = mix(getTerrainNormal(),
                               texture(texOMR, vec3(VAR._texCoord * UNDERWATER_TILE_SCALE, 2)).rgb,
                               saturate(waterData.x));
    // VAR._normalWV is in world-space
    const vec3 perturbedNormal = computeTBN() * (normalMap * 255.f / 127.f - 128.f / 127.f);
    normalWV = normalize(mat3(dvd_ViewMatrix) * perturbedNormal);
#endif //HAS_PRE_PASS_DATA && !LOW_QUALITY

    return vec4(1.f);
#else //PRE_PASS

#if !defined(LOW_QUALITY)
    normalWV = getScreenNormal(VAR._texCoord);
#endif //!LOW_QUALITY
    const vec2 waterData = GetWaterDetails(VAR._vertexW.xyz, TERRAIN_HEIGHT_OFFSET);
    return mix(getTerrainAlbedo(),
               getUnderwaterAlbedo(VAR._texCoord * UNDERWATER_TILE_SCALE, waterData.y),
               saturate(waterData.x));
#endif //PRE_PASS
}
#endif //_TERRAIN_SPLATTING_FRAG_