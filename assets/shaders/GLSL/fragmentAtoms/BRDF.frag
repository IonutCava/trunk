#ifndef _BRDF_FRAG_
#define _BRDF_FRAG_

#include "utility.cmn"
#include "lightInput.cmn"

#include "materialData.frag"
#include "shadowMapping.frag"
#include "pbr.frag"

#define DISABLE_SSR

#if defined(PBR_SHADING)
#define getBRDFFactors(LDir, LCol, OMR, Albedo, N, NdotL) PBR(LDir, LCol, OMR, Albedo, N, NdotL);
#elif defined(USE_SHADING_PHONG) || defined (USE_SHADING_BLINN_PHONG)
#define getBRDFFactors(LDir, LCol, OMR, Albedo, N, NdotL) Phong(LDir, LCol, OMR, Albedo, N, NdotL)
#else
#if defined(USE_SHADING_TOON) // ToDo
#   define getBRDFFactors(LDir, LCol, Data, Albedo, N, NdotL) vec4(0.6f, 0.2f, 0.9f, 0.0f); //obvious pink
#else
#   define getBRDFFactors(LDir, LCol, Data, Albedo, N, NdotL) vec4(0.6f, 1.0f, 0.7f, 0.0f); //obvious lime-green
#endif
#endif

#if !defined(MAX_LIGHTS_PER_PASS)
#define MAX_LIGHTS_PER_PASS MAX_LIGHTS_PER_TILE
#endif

uint GetTileIndex() {
    ivec2 tileID = ivec2(gl_FragCoord.xy) / ivec2(FORWARD_PLUS_TILE_RES, FORWARD_PLUS_TILE_RES);
    return uint(tileID.y * dvd_numTilesX + tileID.x);
}

float TanAcosNdL(in float ndl) {
#if 0
    return saturate(tan(acos(ndl)));
#else
    // Same as above but maybe faster?
    return saturate(sqrt(1.f - ndl * ndl) / ndl);
#endif
}

vec3 getDirectionalLightContribution(in uint dirLightCount, in vec3 albedo, in vec3 occlusionMetallicRoughness, in vec3 normalWV, in bool receivesShadows, in int lodLevel) {
    vec3 ret = vec3(0.0);
    for (uint lightIdx = 0; lightIdx < dirLightCount; ++lightIdx) {
        const Light light = dvd_LightSource[lightIdx];

        const vec3 lightDirectionWV = -light._directionWV.xyz;
        const float ndl = saturate((dot(normalWV, normalize(lightDirectionWV))));
        const float shadowFactor = getShadowFactor(light._options, TanAcosNdL(ndl), receivesShadows, lodLevel);

        vec4 temp = getBRDFFactors(lightDirectionWV,
                                   vec4(light._colour.rgb, 1.f),
                                   occlusionMetallicRoughness,
                                   vec4(albedo, shadowFactor), 
                                   normalWV,
                                   ndl);

        ret += temp.rgb;
    }

    return ret;
}

float getPointAttenuation(in Light light, in vec3 lightDirectionWV) {
    const float radius = light._positionWV.w;
    const float dist = length(lightDirectionWV); 
    const float att = saturate(1.0f - ((dist * dist) / (radius * radius)));
    return att * att;
}

float getSpotAttenuation(in Light light, in vec3 lightDirectionWV, in vec3 lightDirectionWVNorm) {
    const vec3 spotDirectionWV = normalize(light._directionWV.xyz);
    const float cosOuterConeAngle = light._colour.w;
    const float cosInnerConeAngle = light._directionWV.w;

    const float theta = dot(lightDirectionWVNorm, normalize(-spotDirectionWV));
    const float intensity = saturate((theta - cosOuterConeAngle) / (cosInnerConeAngle - cosOuterConeAngle));

    const float radiusSpot = mix(float(light._options.z), light._positionWV.w, 1.0f - intensity);

    const float dist = length(lightDirectionWV);
    const float att = saturate(1.0f - ((dist * dist) / (radiusSpot * radiusSpot)));
    return att * intensity;
}

vec3 getOtherLightContribution(in uint dirLightCount, in vec3 albedo, in vec3 occlusionMetallicRoughness, in vec3 normalWV, in bool receivesShadows, in int lodLevel) {
    const uint offset = GetTileIndex() * MAX_LIGHTS_PER_TILE;

    vec3 ret = vec3(0.0);
    for (uint i = 0; i < MAX_LIGHTS_PER_PASS; ++i) {
        const int lightIdx = perTileLightIndices[offset + i];
        if (lightIdx == -1) {
            break;
        }

        const Light light = dvd_LightSource[lightIdx + dirLightCount];
        const vec3 lightDirectionWV = light._positionWV.xyz - VAR._vertexWV.xyz;
        const vec3 lightDirectionWVNorm = normalize(lightDirectionWV);

        const float ndl = saturate(dot(normalWV, lightDirectionWVNorm));

        const float shadowFactor = getShadowFactor(light._options, TanAcosNdL(ndl), receivesShadows, lodLevel);

        float att = 0.0f;
        if (light._options.x == 1) {
            att = getPointAttenuation(light, lightDirectionWV);
        } else {
            att = getSpotAttenuation(light, lightDirectionWV, lightDirectionWVNorm);
        }

        const vec4 colourAndAtt = vec4(light._colour.rgb, att);
        vec4 temp = getBRDFFactors(lightDirectionWV, colourAndAtt, occlusionMetallicRoughness, vec4(albedo, shadowFactor), normalWV, ndl);
        ret += temp.rgb;
    }

    return ret;
}

float getShadowFactor(in vec3 normalWV, in bool receivesShadows, in int lodLevel) {
    float ret = 1.0f;
    if (receivesShadows) {
        const uint dirLightCount = dvd_LightData.x;

        for (uint lightIdx = 0; lightIdx < dirLightCount; ++lightIdx) {
            const Light light = dvd_LightSource[lightIdx];

            const vec3 lightDirectionWV = -light._directionWV.xyz;
            const float ndl = saturate((dot(normalWV, normalize(lightDirectionWV))));
            ret *= getShadowFactor(dvd_LightSource[lightIdx]._options, TanAcosNdL(ndl), receivesShadows, lodLevel);
        }

        const uint offset = GetTileIndex() * MAX_LIGHTS_PER_TILE;
        for (uint i = 0; i < MAX_LIGHTS_PER_PASS; ++i) {
            const int lightIdx = perTileLightIndices[offset + i];
            if (lightIdx == -1) {
                break;
            }
            const Light light = dvd_LightSource[lightIdx + dirLightCount];

            const vec3 lightDirectionWV = light._positionWV.xyz - VAR._vertexWV.xyz;
            const float ndl = saturate((dot(normalWV, normalize(lightDirectionWV))));
            ret *= getShadowFactor(light._options, TanAcosNdL(ndl), receivesShadows, lodLevel);
        }
    }

    return ret;
}

vec3 lightTileColour() {
    ivec2 loc = ivec2(gl_FragCoord.xy);
    ivec2 tileID = loc / ivec2(FORWARD_PLUS_TILE_RES, FORWARD_PLUS_TILE_RES);
    uint index = tileID.y * dvd_numTilesX + tileID.x;
    uint offset = index * MAX_LIGHTS_PER_TILE;

    uint count = 0;
    for (uint i = 0; i < MAX_LIGHTS_PER_TILE; i++) {
        if (perTileLightIndices[offset + i] == -1) {
            break;
        }

        count++;
    }
    float shade = float(count) / float(MAX_LIGHTS_PER_TILE * 2);
    return vec3(shade);
}

#ifdef CUSTOM_IBL
vec3 ImageBasedLighting(in vec3 colour, in vec3 normalWV, in float metallic, in float roughness, in int textureSize);
#else
vec3 CubeLookup(in vec3 normalWV, in float roughness, in int textureSize) {
    const vec3 normal = normalize(mat3(dvd_InverseViewMatrix) * normalWV);
    const vec3 toCamera = normalize(VAR._vertexW.xyz - dvd_cameraPosition.xyz);
    const vec3 reflection = normalize(reflect(toCamera, normal));

    return  ImageBasedLighting(reflection, normal, toCamera, roughness, textureSize);
}

vec3 ImageBasedLighting(in vec3 colour, in vec3 normalWV, in float metallic, in float roughness, in int textureSize) {
    return mix(colour, CubeLookup(normalWV, roughness, textureSize), metallic);
}
#endif

vec3 getLitColour(in vec3 albedo, in mat4 colourMatrix, in vec3 normalWV, in vec2 uv, in bool receivesShadows, in int lodLevel) {
    const vec3 OMR = getOcclusionMetallicRoughness(colourMatrix, uv);
    switch (dvd_materialDebugFlag) {
        case DEBUG_ALBEDO: return albedo;
        case DEBUG_UV: return vec3(fract(uv), 0.0f);
        case DEBUG_SSAO: return vec3(getSSAO());
        case DEBUG_EMISSIVE: return getEmissive(colourMatrix);
        case DEBUG_ROUGHNESS: return vec3(OMR.b);
        case DEBUG_METALLIC: return vec3(OMR.g);
        case DEBUG_NORMALS: return (dvd_InverseViewMatrix * vec4(normalWV, 0)).xyz;
        case DEBUG_TBN_VIEW_DIRECTION: return getTBNViewDirection();
        case DEBUG_SHADOW_MAPS: return vec3(getShadowFactor(normalWV, receivesShadows, lodLevel));
        case DEBUG_LIGHT_TILES: return lightTileColour();
        case DEBUG_REFLECTIONS: return ImageBasedLighting(vec3(0.f), normalWV, OMR.g, OMR.b, IBLSize(colourMatrix));
    }

    //albedo.rgb += dvd_AmbientColour.rgb;
    albedo.rgb *= getSSAO();
    //albedo.rgb *= OMR.r;

#if defined(USE_SHADING_FLAT)
    return albedo;
#else //USE_SHADING_FLAT

    const uint dirLightCount = dvd_LightData.x;

    vec3 lightColour =
        getEmissive(colourMatrix) +
        getDirectionalLightContribution(dirLightCount, albedo, OMR, normalWV, receivesShadows, lodLevel) +
        getOtherLightContribution(dirLightCount, albedo, OMR, normalWV, receivesShadows, lodLevel);

#if !defined(USE_PLANAR_REFLECTION)
    lightColour.rgb = ImageBasedLighting(lightColour.rgb, normalWV, OMR.g, OMR.b, IBLSize(colourMatrix));
#endif //USE_PLANAR_REFLECTION

    return lightColour;
#endif //USE_SHADING_FLAT
}

vec4 getPixelColour(in vec4 albedo, in NodeData data, in vec3 normalWV, in vec2 uv) {
    const mat4 colourMatrix = data._colourMatrix;
    const int lodLevel = dvd_lodLevel(data);
    const bool receivesShadows = dvd_receivesShadow(data);

    vec4 colour = vec4(getLitColour(albedo.rgb, colourMatrix, normalWV, uv, receivesShadows, lodLevel), albedo.a);
#if !defined(DISABLE_SHADOW_MAPPING)
    // CSM Info
    if (dvd_CSMSplitsViewIndex > -1) {
        switch (getCSMSlice(dvd_CSMShadowTransforms[dvd_CSMSplitsViewIndex].dvd_shadowLightPosition)) {
            case  0: colour.r += 0.15f; break;
            case  1: colour.g += 0.25f; break;
            case  2: colour.b += 0.40f; break;
            case  3: colour.rgb += 1 * vec3(0.15f, 0.25f, 0.40f); break;
            case  4: colour.rgb += 2 * vec3(0.15f, 0.25f, 0.40f); break;
            case  5: colour.rgb += 3 * vec3(0.15f, 0.25f, 0.40f); break;
        };
    }
#endif //DISABLE_SHADOW_MAPPING
    return colour;
}

#endif