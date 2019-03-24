#ifndef _BRDF_FRAG_
#define _BRDF_FRAG_

#if defined(COMPUTE_TBN)
#include "bumpMapping.frag"
#else
#include "lightInput.cmn"
#endif

vec3 private_normalWV = vec3(0.0f);

#include "lightData.frag"
#include "materialData.frag"
#if !defined(DISABLE_SHADOW_MAPPING)
#include "shadowMapping.frag"
#endif

#if defined(USE_SHADING_COOK_TORRANCE) || defined(USE_SHADING_OREN_NAYAR)
#include "pbr.frag"
#define getBRDFFactors(A, B, C, D) PBR(A, B, C, D);
#elif defined(USE_SHADING_PHONG) || defined (USE_SHADING_BLINN_PHONG)
#include "phong_lighting.frag"
#define getBRDFFactors(A, B, C, D) Phong(A, B, C, D)
#else
#if defined(USE_SHADING_TOON) // ToDo
#   define getBRDFFactors(A, B, C, D) vec4(0.6f, 0.2f, 0.9f, 0.0f); //obvious pink
#else
#   define getBRDFFactors(A, B, C, D) vec4(0.6f, 1.0f, 0.7f, 0.0f); //obvious lime-green
#endif
#endif

#if !defined(MAX_LIGHTS_PER_PASS)
#define MAX_LIGHTS_PER_PASS MAX_NUM_LIGHTS_PER_TILE
#endif

vec4 getDirectionalLightContribution(in uint dirLightCount, in vec4 albedoAndShadow, in vec4 specular) {
    vec4 ret = vec4(0.0f);

    for (uint lightIdx = 0; lightIdx < dirLightCount; ++lightIdx) {
        const Light light = dvd_LightSource[lightIdx];
        ret += getBRDFFactors(vec4(light._colour.rgb, 1.0f), specular, albedoAndShadow , -light._directionWV.xyz);
    }

    return ret;
}

vec4 getPointLightContribution(in uint tileIndex, in uint offset, in vec4 albedoAndShadow, in vec4 specular) {
    vec4 ret = vec4(0.0f);

    uint tileOffset = tileIndex * MAX_NUM_LIGHTS_PER_TILE;
    for (uint i = 0; i < MAX_LIGHTS_PER_PASS; ++i) {
        int lightIdx = perTileLightIndices[tileOffset + i];
        if (lightIdx == -1) {
            return ret;
        }

        const Light light = dvd_LightSource[lightIdx + offset];

        const vec3 lightDirection = light._positionWV.xyz - VAR._vertexWV.xyz;
        const vec4 colourAndAtt = vec4(light._colour.rgb, getLightAttenuationPoint(light, lightDirection));

        ret += getBRDFFactors(colourAndAtt, specular, albedoAndShadow, lightDirection);
    }

    return ret;
}

vec4 getSpotLightContribution(in uint tileIndex, in uint offset, in uint pointLightCount, in vec4 albedoAndShadow, in vec4 specular) {
    vec4 ret = vec4(0.0f);

    uint tileOffset = tileIndex * MAX_NUM_LIGHTS_PER_TILE;
    for (uint i = 0; i < MAX_LIGHTS_PER_PASS; ++i) {
        int lightIdx = perTileLightIndices[tileOffset + i + pointLightCount];
        if (lightIdx == -1) {
            return ret;
        }

        const Light light = dvd_LightSource[lightIdx + offset];
        const vec3 lightDirection = light._positionWV.xyz - VAR._vertexWV.xyz;
        const vec4 colourAndAtt = vec4(light._colour.rgb, getLightAttenuationSpot(light, lightDirection));

        ret += getBRDFFactors(colourAndAtt, specular, albedoAndShadow, lightDirection);
    }

    return ret;
}

#if defined(USE_SHADING_FLAT)

vec4 getLitColour(in vec4 albedo, in mat4 colourMatrix, in vec3 normal) {
    return albedo;
}

#else //USE_SHADING_FLAT

vec3 getLitColour(in vec3 albedo, in mat4 colourMatrix, in vec3 normal, in float shadowFactor) {
    private_normalWV = normal;

    const uint dirLightCount = dvd_LightData.x;
    const vec4 specular = vec4(getSpecular(colourMatrix), getReflectivity(colourMatrix));

    // Apply all lighting contributions (.a = reflectionCoeff)
    vec4 lightColour = getDirectionalLightContribution(dirLightCount, vec4(albedo, shadowFactor), specular);

    if (dvd_lodLevel < 2)
    {
         uint tileIndex = GetTileIndex(gl_FragCoord.xy);
         lightColour += getPointLightContribution(tileIndex, dirLightCount, vec4(albedo, shadowFactor), specular);
         // Move past the first sentinel to get to the spot lights
         //lightColour += getSpotLightContribution(tileIndex, dirLightCount, dvd_LightData.y, vec4(albedo, shadowFactor), specular);
    }

    lightColour.rgb += getEmissive(colourMatrix);

    if (dvd_lodLevel < 1 && getReflectivity(colourMatrix) > 100) {
        vec3 reflectDirection = reflect(normalize(VAR._vertexWV.xyz), normal);
        reflectDirection = vec3(inverse(dvd_ViewMatrix) * vec4(reflectDirection, 0.0f));
        /*lightColour.rgb = mix(texture(texEnvironmentCube, vec4(reflectDirection, dvd_reflectionIndex)).rgb,
                          lightColour.rgb,
                          vec3(saturate(lightColour.a)));
        */
    }

    return lightColour.rgb * shadowFactor;
}
#endif //USE_SHADING_FLAT

vec4 getPixelColour(in vec4 albedo, in mat4 colourMatrix, in vec3 normal) {
#if defined(OIT_PASS)
    const float shadowFactor = 1.0f;
#else
    const float shadowFactor = getShadowFactor();
#endif

    vec4 colour = vec4(getLitColour(albedo.rgb, colourMatrix, normal, shadowFactor), albedo.a);

#if !defined(DISABLE_SHADOW_MAPPING) && defined(DEBUG_SHADOWMAPPING)
    if (dvd_showDebugInfo) {
        switch (getShadowData()) {
            case -1: colour.rgb = vec3(1.0f); break;
            case  0: colour.r += 0.15f; break;
            case  1: colour.g += 0.25f; break;
            case  2: colour.b += 0.40f; break;
            case  3: colour.rgb += vec3(0.15f, 0.25f, 0.40f); break;
        };
    }
#endif //DEBUG_SHADOWMAPPING

    return colour;
}

vec3 getNormal() {
    vec3 normal = VAR._normalWV;

#if defined(COMPUTE_TBN)
    if (dvd_lodLevel == 0) {
#   if defined(USE_PARALLAX_MAPPING)
        normal = ParallaxNormal(VAR._texCoord, 0);
#   elif defined(USE_RELIEF_MAPPING)
#   else
        normal = getTBNMatrix() * getBump(VAR._texCoord);
#   endif //USE_PARALLAX_MAPPING
    }
#endif //COMPUTE_TBN

#   if defined (USE_DOUBLE_SIDED)
    if (!gl_FrontFacing) {
        normal = -normal;
    }
#   endif //USE_DOUBLE_SIDED

    return normal;
}

#endif