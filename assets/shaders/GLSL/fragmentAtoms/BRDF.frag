#ifndef _BRDF_FRAG_
#define _BRDF_FRAG_

#include "lightInput.cmn"

#include "materialData.frag"
#include "shadowMapping.frag"
#include "pbr.frag"

#define DISABLE_SSR


float TanAcosNdL(in float ndl) {
#if 0
    return saturate(tan(acos(ndl)));
#else
    // Same as above but maybe faster?
    return saturate(sqrt(1.f - ndl * ndl) / ndl);
#endif
}

float GetNdotL(in vec3 N, in vec3 L) {
    const float ndl = clamp(dot(N, L), M_EPSILON, 1.0f);
    return ndl;
}

vec3 getLightContribution(in vec3 albedo, in vec3 OMR, in vec3 normalWV)
{
    vec3 ret = vec3(0.0);
    float specularFactor = 0.0f;

    const vec3 specColour = SpecularColour(albedo, METALLIC(OMR));
    const vec3 toCamera = normalize(VAR._viewDirectionWV);
    const float roughness = ROUGHNESS(OMR);

    const uint dirLightCount = dvd_LightData.x;
    for (uint lightIdx = 0; lightIdx < dirLightCount; ++lightIdx) {
        const Light light = dvd_LightSource[lightIdx];

        const vec3 lightDir = -light._directionWV.xyz;
        const vec3 lightVec = lightDir;

        const float ndl = GetNdotL(normalWV, lightVec);

        const float shadowFactor = getShadowFactorDirectional(light._options.y, TanAcosNdL(ndl));
        if (shadowFactor > M_EPSILON) {
            float tempSpecularFactor = 0.0f;
            vec3 BRDF = GetBRDF(lightDir,
                                lightVec,
                                toCamera,
                                normalWV,
                                albedo, 
                                specColour,
                                ndl,
                                roughness,
                                tempSpecularFactor);

            ret += BRDF * light._colour.rgb * shadowFactor;

            specularFactor = max(specularFactor, tempSpecularFactor);
        }
    }

    LightGrid grid = lightGrid[GetClusterIndex(gl_FragCoord)];

    uint lightIndexOffset = grid.offset;
    uint lightCount  = grid.countPoint;
    for (uint i = 0; i < lightCount; ++i) {
        const uint lightIdx = globalLightIndexList[lightIndexOffset + i] + dirLightCount;

        const Light light = dvd_LightSource[lightIdx];
        const vec3 lightDir = light._positionWV.xyz - VAR._vertexWV.xyz;
        const vec3 lightVec = normalize(lightDir);

        const float ndl = GetNdotL(normalWV, lightVec);

        const float shadowFactor = getShadowFactorPoint(light._options.y, TanAcosNdL(ndl));
        if (shadowFactor > M_EPSILON) {
            const float radiusSQ = SQUARED(light._positionWV.w);
            const float dist = length(lightDir);
            const float att = saturate(1.0f - (SQUARED(dist) / radiusSQ));

            float tempSpecularFactor = 0.0f;
            vec3 BRDF = GetBRDF(lightDir,
                                lightVec,
                                toCamera,
                                normalWV,
                                albedo,
                                specColour,
                                ndl,
                                roughness,
                                tempSpecularFactor);

            ret += BRDF * light._colour.rgb * SQUARED(att) * shadowFactor;

            specularFactor = max(specularFactor, tempSpecularFactor);
        }
    }

    lightIndexOffset += lightCount;
    lightCount = grid.countSpot;
    for (uint i = 0; i < lightCount; ++i) {
        const uint lightIdx = globalLightIndexList[lightIndexOffset + i] + dirLightCount;

        const Light light = dvd_LightSource[lightIdx];
        const vec3 lightDir = light._positionWV.xyz - VAR._vertexWV.xyz;
        const vec3 lightVec = normalize(lightDir);

        const float ndl = GetNdotL(normalWV, lightVec);

        const float shadowFactor = getShadowFactorSpot(light._options.y, TanAcosNdL(ndl));
        if (shadowFactor > M_EPSILON) {
            const vec3  spotDirectionWV = normalize(light._directionWV.xyz);
            const float cosOuterConeAngle = light._colour.w;
            const float cosInnerConeAngle = light._directionWV.w;

            const float theta = dot(lightVec, normalize(-spotDirectionWV));
            const float intensity = saturate((theta - cosOuterConeAngle) / (cosInnerConeAngle - cosOuterConeAngle));

            const float dist = length(lightDir);
            const float radius = mix(float(light._SPOT_CONE_SLANT_HEIGHT), light._positionWV.w, 1.0f - intensity);
            float att = saturate(1.0f - (SQUARED(dist) / SQUARED(radius)));
            att = att * intensity;

            float tempSpecularFactor = 0.0f;
            vec3 BRDF = GetBRDF(lightDir,
                                lightVec,
                                toCamera,
                                normalWV,
                                albedo,
                                specColour,
                                ndl,
                                roughness,
                                tempSpecularFactor);

            ret += BRDF * light._colour.rgb * att * shadowFactor;

            specularFactor = max(specularFactor, tempSpecularFactor);
        }
    }

    return ret;
}

float getShadowFactor(in vec3 normalWV) {
    float ret = 1.0f;
    const uint dirLightCount = dvd_LightData.x;

    for (uint lightIdx = 0; lightIdx < dirLightCount; ++lightIdx) {
        const Light light = dvd_LightSource[lightIdx];

        const vec3 lightDirectionWV = -light._directionWV.xyz;
        const float ndl = saturate((dot(normalWV, normalize(lightDirectionWV))));
        ret *= getShadowFactorDirectional(dvd_LightSource[lightIdx]._options.y, TanAcosNdL(ndl));
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
        ret *= getShadowFactorPoint(light._options.y, TanAcosNdL(ndl));
    }

    lightIndexOffset += lightCountPoint;
    for (uint i = 0; i < lightCountSpot; i++) {
        const uint lightIdx = globalLightIndexList[lightIndexOffset + i] + dirLightCount;

        const Light light = dvd_LightSource[lightIdx];

        const vec3 lightDirectionWV = light._positionWV.xyz - VAR._vertexWV.xyz;
        const float ndl = saturate((dot(normalWV, normalize(lightDirectionWV))));
        ret *= getShadowFactorSpot(light._options.y, TanAcosNdL(ndl));
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

#if !defined(CUSTOM_IBL)
vec3 ImageBasedLighting(in vec3 colour, in vec3 normalWV, in float metallic, in float roughness, in uint textureSize, in uint probeIdx)
{
#if defined(USE_PLANAR_REFLECTION) || defined(NO_IBL)
    return colour;
#else //USE_PLANAR_REFLECTION) || NO_IBL
    return mix(colour,
               IBL(
                   normalize(mat3(dvd_InverseViewMatrix) * normalWV),
                   roughness,
                   textureSize,
                   probeIdx
               ),
               metallic);
#endif  //USE_PLANAR_REFLECTION) || NO_IBL
}
#endif //CUSTOM_IBL

/// returns RGB - pixel lit colour, A - reflectivity (e.g. for SSR)
#if defined(DISABLE_SHADOW_MAPPING)
#define CSMSplitColour() vec4(0.0f)
#else //DISABLE_SHADOW_MAPPING
vec4 CSMSplitColour() {
    vec4 colour = vec4(0.0f);

    int shadowIndex = -1;
    const uint dirLightCount = dvd_LightData.x;
    for (uint lightIdx = 0; lightIdx < dirLightCount; ++lightIdx) {
        const Light light = dvd_LightSource[lightIdx];
        shadowIndex = dvd_LightSource[lightIdx]._options.y;
    }
    if (shadowIndex > -1) {
        switch (getCSMSlice(dvd_CSMShadowTransforms[shadowIndex].dvd_shadowLightPosition)) {
            case  0: colour.r += 0.15f; break;
            case  1: colour.g += 0.25f; break;
            case  2: colour.b += 0.40f; break;
            case  3: colour.rgb += 1 * vec3(0.15f, 0.25f, 0.40f); break;
            case  4: colour.rgb += 2 * vec3(0.15f, 0.25f, 0.40f); break;
            case  5: colour.rgb += 3 * vec3(0.15f, 0.25f, 0.40f); break;
        };
    };
    return colour;
}
#endif //DISABLE_SHADOW_MAPPING

#if !defined(NO_FOG)
//https://iquilezles.org/www/articles/fog/fog.htm
vec3 applyFog(in vec3  rgb,      // original color of the pixel
              in float distance, // camera to point distance
              in vec3  rayOri,   // camera position
              in vec3  rayDir)   // camera to point vector
{
    const float c = dvd_fogDetails._colourSunScatter.a;
    const float b = dvd_fogDetails._colourAndDensity.a;
    const float fogAmount = c * exp(-rayOri.y * b) * (1.f - exp(-distance * rayDir.y * b)) / rayDir.y;
    return mix(rgb, dvd_fogDetails._colourAndDensity.rgb, fogAmount);
}
#endif //!NO_FOG

/// RGB - lit colour, A - reflectivity
vec4 getPixelColour(in vec4 albedo, in NodeMaterialData materialData, in vec3 normalWV, in vec2 uv, in uint LoD) {
    const vec3 OMR = getOcclusionMetallicRoughness(materialData, uv);

    switch (dvd_materialDebugFlag) {
        case DEBUG_ALBEDO:         return vec4(albedo.rgb, 1.0f);
        case DEBUG_DEPTH:          return vec4(vec3(ToLinearDepthPreview(texture(texDepthMap, dvd_screenPositionNormalised).r)), 1.0f);
        case DEBUG_LIGHTING:       return vec4(getLightContribution(vec3(0.0f), OMR, normalWV), 1.0f);
        case DEBUG_SPECULAR:       return vec4(SpecularColour(albedo.rgb, METALLIC(OMR)), 0.0f);
        case DEBUG_UV:             return vec4(fract(uv), 0.0f, 0.0f);
        case DEBUG_SSAO:           return vec4(vec3(SSAOFactor), 1.0f);
        case DEBUG_EMISSIVE:       return vec4(EmissiveColour(materialData), 1.0f);
        case DEBUG_ROUGHNESS:      return vec4(vec3(ROUGHNESS(OMR)), 1.0f);
        case DEBUG_METALLIC:       return vec4(vec3(METALLIC(OMR)), 1.0f);
        case DEBUG_NORMALS:        return vec4(normalize(mat3(dvd_InverseViewMatrix) * normalWV), 1.0f);
        case DEBUG_TANGENTS:       return vec4(normalize(mat3(dvd_InverseViewMatrix) * getTangentWV()), 1.f);
        case DEBUG_BITANGENTS:     return vec4(normalize(mat3(dvd_InverseViewMatrix) * getBiTangentWV()), 1.f);
        case DEBUG_SHADOW_MAPS:    return vec4(vec3(getShadowFactor(normalWV)), 1.0f);
        case DEBUG_CSM_SPLITS:     return albedo + CSMSplitColour();
        case DEBUG_LIGHT_HEATMAP:  return vec4(lightClusterColours(false), 1.0f);
        case DEBUG_DEPTH_CLUSTERS: return vec4(lightClusterColours(true), 1.0f);
        case DEBUG_REFLECTIONS:    return vec4(ImageBasedLighting(vec3(0.f), normalWV, METALLIC(OMR), ROUGHNESS(OMR), IBLSize(materialData), dvd_probeIndex(materialData)), 1.0f);
        case DEBUG_REFLECTIVITY:   return vec4(vec3(mix(0.0f, 1.0f - ROUGHNESS(OMR), METALLIC(OMR))), 1.0f);
        case DEBUG_MATERIAL_IDS:   return vec4(turboColormap(float(MATERIAL_IDX + 1) / MAX_CONCURRENT_MATERIALS), 1.0f);
    }

#if defined(USE_SHADING_FLAT)
    vec3 oColour = albedo.rgb;
#else //USE_SHADING_FLAT
    vec3 oColour = getLightContribution(albedo.rgb, OMR, normalWV);
#endif //USE_SHADING_FLAT

    oColour = ImageBasedLighting(oColour, normalWV, METALLIC(OMR), ROUGHNESS(OMR), IBLSize(materialData), dvd_probeIndex(materialData));
#if defined(SSAO_LOD_0)
    if (LoD < 2) {
#endif //SSAO_LOD_0
        oColour *= SSAOFactor;
#if defined(SSAO_LOD_0)
    }
#endif//SSAO_LOD_0
    oColour += EmissiveColour(materialData);
#if !defined(NO_FOG)
    oColour = applyFog(oColour, distance(VAR._vertexW.xyz, dvd_cameraPosition.xyz), dvd_cameraPosition.xyz, normalize(VAR._vertexW.xyz - dvd_cameraPosition.xyz));
#endif //!NO_FOG

#if defined(OIT_PASS)
    // alpha for WOIT
    const float extraData = albedo.a;
#else
    const float smoothness = 1.0f - ROUGHNESS(OMR);
    // reflectivity (for SSR)
    const float extraData = mix(0.0f, smoothness, METALLIC(OMR));
#endif

    return vec4(oColour, extraData);
}

#endif //_BRDF_FRAG_