#ifndef _BRDF_FRAG_
#define _BRDF_FRAG_

#include "lightInput.cmn"

#include "lightData.frag"
#include "materialData.frag"
#include "shadowMapping.frag"

#if defined(USE_SHADING_COOK_TORRANCE) || defined(USE_SHADING_OREN_NAYAR)
#include "pbr.frag"
#define getBRDFFactors(A, B, C, D, E) PBR(A, B, C, D, E);
#elif defined(USE_SHADING_PHONG) || defined (USE_SHADING_BLINN_PHONG)
#include "phong_lighting.frag"
#define getBRDFFactors(A, B, C, D, E) Phong(A, B, C, D, E)
#else
#if defined(USE_SHADING_TOON) // ToDo
#   define getBRDFFactors(A, B, C, D, E) vec4(0.6f, 0.2f, 0.9f, 0.0f); //obvious pink
#else
#   define getBRDFFactors(A, B, C, D, E) vec4(0.6f, 1.0f, 0.7f, 0.0f); //obvious lime-green
#endif
#endif

#if !defined(MAX_LIGHTS_PER_PASS)
#define MAX_LIGHTS_PER_PASS MAX_NUM_LIGHTS_PER_TILE
#endif

void getDirectionalLightContribution(in uint dirLightCount, in vec3 albedo, in vec4 specular, in vec3 normalWV, inout vec4 lightColour) {
    const vec3 ambient = vec3(0.01f, 0.01f, 0.01f);

    for (uint lightIdx = 0; lightIdx < dirLightCount; ++lightIdx) {
        const Light light = dvd_LightSource[lightIdx];
        lightColour += getBRDFFactors(vec4(light._colour.rgb, 1.0f), specular, vec4(albedo, getShadowFactor(light._options.y)), normalWV, getLightDirection(light));

        lightColour.rgb += (ambient * when_lt(lightColour.a, 0.01f)) * when_eq(lightIdx, 0);
    }
}

void getPointLightContribution(in uint tileIndex, in uint dirLightCount, in vec3 albedo, in vec4 specular, in vec3 normalWV, inout vec4 lightColour) {
    const uint tileOffset = (tileIndex * MAX_NUM_LIGHTS_PER_TILE);

    for (uint i = 0; i < MAX_LIGHTS_PER_PASS; ++i) {
        const int lightIdx = perTileLightIndices[tileOffset + i];
        if (lightIdx == -1) {
            break;
        }

        const Light light = dvd_LightSource[lightIdx + dirLightCount];
        const vec4 colourAndAtt = vec4(light._colour.rgb, getLightAttenuationPoint(light));

        lightColour += getBRDFFactors(colourAndAtt, specular, vec4(albedo, getShadowFactor(light._options.y)), normalWV, getLightDirection(light));
    }
}

void getSpotLightContribution(in uint tileIndex, in uint dirLightCount, in uint pointLightCount, in vec3 albedo, in vec4 specular, in vec3 normalWV, inout vec4 lightColour) {
    const uint tileOffset = (tileIndex * MAX_NUM_LIGHTS_PER_TILE) + pointLightCount;

    for (uint i = 0; i < MAX_LIGHTS_PER_PASS; ++i) {
        const int lightIdx = perTileLightIndices[tileOffset + i];
        if (lightIdx == -1) {
            break;
        }

        const Light light = dvd_LightSource[lightIdx + dirLightCount];
        const vec4 colourAndAtt = vec4(light._colour.rgb, getLightAttenuationSpot(light));

        lightColour += getBRDFFactors(colourAndAtt, specular, vec4(albedo, getShadowFactor(light._options.y)), normalWV, getLightDirection(light));
    }
}

vec3 getLitColour(in vec3 albedo, in mat4 colourMatrix, in vec3 normal, in vec2 uv) {
#if defined(USE_SHADING_FLAT)
    return albedo;
#else //USE_SHADING_FLAT
    const uint dirLightCount = dvd_LightData.x;
    const uint pointLightCount = dvd_LightData.y;

    const vec4 specular = vec4(getSpecular(colourMatrix, uv), getReflectivity(colourMatrix));

    // Apply all lighting contributions (.a = reflectionCoeff)
    vec4 lightColour = vec4(0.0f);
    getDirectionalLightContribution(dirLightCount, albedo, specular, normal, lightColour);

    if (dvd_lodLevel < 2)
    {
         const uint tileIndex = GetTileIndex(gl_FragCoord.xy);
         getPointLightContribution(tileIndex, dirLightCount, albedo, specular, normal, lightColour);
         // Move past the first sentinel to get to the spot lights
         getSpotLightContribution(tileIndex, dirLightCount, pointLightCount, albedo, specular, normal, lightColour);
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

    return lightColour.rgb;
#endif //USE_SHADING_FLAT
}

vec4 getPixelColour(in vec4 albedo, in mat4 colourMatrix, in vec3 normal, in vec2 uv) {
    vec4 colour = vec4(getLitColour(albedo.rgb, colourMatrix, normal, uv), albedo.a);

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

#endif