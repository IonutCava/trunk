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


float TanAcosNdL(in float ndl) {
#if 0
    return saturate(tan(acos(ndl)));
#else
    // Same as above but maybe faster?
    return saturate(sqrt(1.f - ndl * ndl) / ndl);
#endif
}

vec3 getLightContribution(in vec3 albedo, in vec3 occlusionMetallicRoughness, in vec3 normalWV, in bool receivesShadows, in int lodLevel) {
    const uint dirLightCount = dvd_LightData.x;

    vec4 ret = vec4(0.0);
    for (uint lightIdx = 0; lightIdx < dirLightCount; ++lightIdx) {
        const Light light = dvd_LightSource[lightIdx];

        const vec3 lightDirectionWV = -light._directionWV.xyz;
        const float ndl = saturate((dot(normalWV, normalize(lightDirectionWV))));
        const float shadowFactor = receivesShadows ? getShadowFactorDirectional(light._options.y, TanAcosNdL(ndl), lodLevel) : 1.0f;

        ret += getBRDFFactors(lightDirectionWV,
                              vec4(light._colour.rgb, 1.f),
                              occlusionMetallicRoughness,
                              vec4(albedo, shadowFactor), 
                              normalWV,
                              ndl);
    }

    LightGrid grid = lightGrid[GetClusterIndex(gl_FragCoord)];

          uint lightIndexOffset = grid.offset;
    const uint lightCountPoint  = grid.countPoint;
    const uint lightCountSpot   = grid.countSpot;

    for (uint i = 0; i < lightCountPoint; ++i) {
        const uint lightIdx = globalLightIndexList[lightIndexOffset + i] + dirLightCount;

        const Light light = dvd_LightSource[lightIdx];
        const vec3 lightDirectionWV = light._positionWV.xyz - VAR._vertexWV.xyz;
        const vec3 lightDirectionWVNorm = normalize(lightDirectionWV);

        const float ndl = saturate(dot(normalWV, lightDirectionWVNorm));

        const float radius = light._positionWV.w;
        const float dist = length(lightDirectionWV);
        const float att = saturate(1.0f - ((dist * dist) / (radius * radius)));

        const float shadowFactor = receivesShadows ? getShadowFactorPoint(light._options.y, TanAcosNdL(ndl), lodLevel) : 1.0f;

        ret += getBRDFFactors(lightDirectionWV, 
                              vec4(light._colour.rgb, att * att),
                              occlusionMetallicRoughness,
                              vec4(albedo, shadowFactor),
                              normalWV,
                              ndl);
    }

    lightIndexOffset += lightCountPoint;
    for (uint i = 0; i < lightCountSpot; ++i) {
        const uint lightIdx = globalLightIndexList[lightIndexOffset + i] + dirLightCount;

        const Light light = dvd_LightSource[lightIdx];
        const vec3 lightDirectionWV = light._positionWV.xyz - VAR._vertexWV.xyz;
        const vec3 lightDirectionWVNorm = normalize(lightDirectionWV);

        const float ndl               = saturate(dot(normalWV, lightDirectionWVNorm));
        const vec3  spotDirectionWV   = normalize(light._directionWV.xyz);
        const float cosOuterConeAngle = light._colour.w;
        const float cosInnerConeAngle = light._directionWV.w;

        const float theta = dot(lightDirectionWVNorm, normalize(-spotDirectionWV));
        const float intensity = saturate((theta - cosOuterConeAngle) / (cosInnerConeAngle - cosOuterConeAngle));

        const float radius = mix(float(light._SPOT_CONE_SLANT_HEIGHT), light._positionWV.w, 1.0f - intensity);

        const float dist = length(lightDirectionWV);
        const float att = saturate(1.0f - ((dist * dist) / (radius * radius))) * intensity;

        const float shadowFactor = receivesShadows ? getShadowFactorSpot(light._options.y, TanAcosNdL(ndl), lodLevel) : 1.0f;

        ret += getBRDFFactors(lightDirectionWV,
                              vec4(light._colour.rgb, att),
                              occlusionMetallicRoughness,
                              vec4(albedo, shadowFactor),
                              normalWV,
                              ndl);
    }

    return ret.rgb;
}

float getShadowFactor(in vec3 normalWV, in bool receivesShadows, in int lodLevel) {
    if (!receivesShadows) {
        return 1.0f;
    }

    float ret = 1.0f;
    const uint dirLightCount = dvd_LightData.x;

    for (uint lightIdx = 0; lightIdx < dirLightCount; ++lightIdx) {
        const Light light = dvd_LightSource[lightIdx];

        const vec3 lightDirectionWV = -light._directionWV.xyz;
        const float ndl = saturate((dot(normalWV, normalize(lightDirectionWV))));
        ret *= getShadowFactorDirectional(dvd_LightSource[lightIdx]._options.y, TanAcosNdL(ndl), lodLevel);
    }

    const uint cluster = GetClusterIndex(gl_FragCoord);

            uint lightIndexOffset = lightGrid[cluster].offset;
    const uint lightCountPoint  = lightGrid[cluster].countPoint;
    const uint lightCountSpot   = lightGrid[cluster].countSpot;

    for (uint i = 0; i < lightCountPoint; i++) {
        const uint lightIdx = globalLightIndexList[lightIndexOffset + i] + dirLightCount;

        const Light light = dvd_LightSource[lightIdx];

        const vec3 lightDirectionWV = light._positionWV.xyz - VAR._vertexWV.xyz;
        const float ndl = saturate((dot(normalWV, normalize(lightDirectionWV))));
        ret *= getShadowFactorPoint(light._options.y, TanAcosNdL(ndl), lodLevel);
    }

    lightIndexOffset += lightCountPoint;
    for (uint i = 0; i < lightCountSpot; i++) {
        const uint lightIdx = globalLightIndexList[lightIndexOffset + i] + dirLightCount;

        const Light light = dvd_LightSource[lightIdx];

        const vec3 lightDirectionWV = light._positionWV.xyz - VAR._vertexWV.xyz;
        const float ndl = saturate((dot(normalWV, normalize(lightDirectionWV))));
        ret *= getShadowFactorSpot(light._options.y, TanAcosNdL(ndl), lodLevel);
    }

    return ret;
}

vec3 lightClusterColours(const bool debugDepthClusters) {
    if (debugDepthClusters) {
        const uint zTile = GetClusterZIndex(gl_FragCoord.z);

        switch (zTile % 8) {
            case 0:  return vec3(1.0f, 0.0f, 0.0f);
            case 1:  return vec3(0.0f, 1.0f, 0.0f);
            case 2:  return vec3(0.0f, 0.0f, 1.0f);
            case 3:  return vec3(1.0f, 1.0f, 0.0f);

            case 4:  return vec3(1.0f, 0.0f, 1.0f);
            case 5:  return vec3(1.0f, 1.0f, 1.0f);
            case 6:  return vec3(1.0f, 0.5f, 0.5f);
            case 7:  return vec3(0.0f, 0.0f, 0.0f);
        }

        return vec3(0.5f, 0.25f, 0.75f);
    }

    const uint cluster = GetClusterIndex(gl_FragCoord);

    uint lights = lightGrid[cluster].countPoint;

    // show possible clipping
    if (lights == 0) {
        lights--;
    } else if (lights == MAX_LIGHTS_PER_CLUSTER) {
        lights++;
    }

    const vec3 colour = turboColormap(float(lights) / MAX_LIGHTS_PER_CLUSTER);

    return colour;
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
        case DEBUG_TBN_VIEW_DIRECTION: return getTBNViewDir();
        case DEBUG_SHADOW_MAPS: return vec3(getShadowFactor(normalWV, receivesShadows, lodLevel));
        case DEBUG_LIGHT_HEATMAP: return lightClusterColours(false);
        case DEBUG_LIGHT_DEPTH_CLUSTERS: return lightClusterColours(true);
        case DEBUG_REFLECTIONS: return ImageBasedLighting(vec3(0.f), normalWV, OMR.g, OMR.b, IBLSize(colourMatrix));
    }

    //albedo.rgb += dvd_AmbientColour.rgb;
    albedo.rgb *= getSSAO();

#if defined(USE_SHADING_FLAT)
    return getEmissive(colourMatrix) + albedo;
#else //USE_SHADING_FLAT

    vec3 lightColour =
        getEmissive(colourMatrix) +
        getLightContribution(albedo, OMR, normalWV, receivesShadows, lodLevel);

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