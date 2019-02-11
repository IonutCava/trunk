#ifndef _BRDF_FRAG_
#define _BRDF_FRAG_

#include "lightInput.cmn"
#include "lightData.frag"
#include "materialData.frag"
#if !defined(DISABLE_SHADOW_MAPPING)
#include "shadowMapping.frag"
#endif
#include "pbr.frag"
#include "phong_lighting.frag"

void getBRDFFactors(in vec3 lightColour,
                    in vec3 lightDirection,
                    in float attenuation,
                    in vec3 normalWV,
                    in vec3 albedo,
                    in vec3 specular,
                    in float reflectivity, //PBR: roughness. Phong: shininess
                    inout vec3 colourInOut,
                    inout float reflectionCoeff)
{

#if defined(USE_SHADING_PHONG) || defined (USE_SHADING_BLINN_PHONG)
    Phong(lightColour, lightDirection, attenuation, normalWV, albedo, specular, reflectivity, colourInOut, reflectionCoeff);
#elif defined(USE_SHADING_TOON)
    // ToDo
    reflectionCoeff = 0;
    colourInOut = vec3(0.6, 0.2, 0.9); //obvious pink
#elif defined(USE_SHADING_COOK_TORRANCE) || defined(USE_SHADING_OREN_NAYAR)
    PBR(lightColour, lightDirection, attenuation, normalWV, albedo, specular, reflectivity, colourInOut, reflectionCoeff);
#else
    reflectionCoeff = 0;
    colourInOut = vec3(0.6, 1.0, 0.7); //obvious lime-green
#endif
}

vec3 getLightColour(vec3 albedo, vec3 normal, mat4 colourMatrix, uint dirLightCount, uint pointLightCount) {
#   if defined(USE_SHADING_FLAT)
    return albedo;
#   else
    float reflectionCoeff = 0.0;
    vec3 lightColour = vec3(0.0);
    vec3 specular = getSpecular(colourMatrix);
    float reflectivity = getReflectivity(colourMatrix);
    // Apply all lighting contributions

    // Directional lights

    for (int lightIdx = 0; lightIdx < dirLightCount; ++lightIdx) {
        Light light = dvd_LightSource[lightIdx];
        getBRDFFactors(light._colour.rgb, -light._directionWV.xyz, 1.0, normal, albedo, specular, reflectivity, lightColour, reflectionCoeff);
    }

    if (dvd_lodLevel < 2) {
        // Point lights

        uint offset = dirLightCount;
        uint nIndex = GetTileIndex(gl_FragCoord.xy) * LIGHT_NUM_LIGHTS_PER_TILE;

        uint nNextLightIndex = perTileLightIndices[nIndex];
        while (nNextLightIndex != LIGHT_INDEX_BUFFER_SENTINEL) {
            Light light = dvd_LightSource[int(nNextLightIndex - 1 + offset)];
            vec3 lightDirection = light._positionWV.xyz - VAR._vertexWV.xyz;

            getBRDFFactors(light._colour.rgb, lightDirection, getLightAttenuationPoint(light, lightDirection), normal, albedo, specular, reflectivity, lightColour, reflectionCoeff);
            nNextLightIndex = perTileLightIndices[++nIndex];
        }

        // Spot lights
        offset += pointLightCount;
        // Moves past the first sentinel to get to the spot lights.
        nNextLightIndex = perTileLightIndices[++nIndex];
        while (nNextLightIndex != LIGHT_INDEX_BUFFER_SENTINEL) {
            Light light = dvd_LightSource[int(nNextLightIndex - 1 + offset)];
            vec3 lightDirection = light._positionWV.xyz - VAR._vertexWV.xyz;

            getBRDFFactors(light._colour.rgb, lightDirection, getLightAttenuationSpot(light, lightDirection), normal, albedo, specular, reflectivity, lightColour, reflectionCoeff);
            nNextLightIndex = perTileLightIndices[++nIndex];
        }
    }

    return mix(getEmissive(colourMatrix), lightColour, DIST_TO_ZERO(length(lightColour)));
#endif
}


vec4 getPixelColour(vec4 albedo, mat4 colourMatrix, inout vec3 normal) {

#   if defined (USE_DOUBLE_SIDED)
    if (!gl_FrontFacing) {
        normal = -normal;
    }
#   endif

#if defined(_DEBUG)
    if (dvd_NormalsOnly) {
        return vec4(normal, 1.0);
    }
    if (dvd_LightingOnly) {
        albedo.rgb = vec3(0.0);
    }
#endif

    vec3 colour = getLightColour(albedo.rgb, normal, colourMatrix, dvd_LightData.x, dvd_LightData.y);

#if defined(IS_REFLECTIVE)
        /*if (dvd_lodLevel < 1) {
            vec3 reflectDirection = reflect(normalize(VAR._vertexWV.xyz), processedNormal);
            reflectDirection = vec3(inverse(dvd_ViewMatrix) * vec4(reflectDirection, 0.0));
            colour = mix(texture(texEnvironmentCube, vec4(reflectDirection, dvd_reflectionIndex)).rgb,
                        colour,
                        vec3(reflectionCoeff));
        }*/

#endif

// Apply shadowing
#if !defined(DISABLE_SHADOW_MAPPING)
    vec4 outColour = vec4(0.0);
    if (dvd_receivesShadows) {
        outColour = vec4(colour * getShadowFactor(), albedo.a);
#       if defined(DEBUG_SHADOWMAPPING)
        if (dvd_showDebugInfo) {
            switch (g_shadowTempInt) {
                case -1: outColour = vec4(1.0); break;
                case  0: outColour.r += 0.15; break;
                case  1: outColour.g += 0.25; break;
                case  2: outColour.b += 0.40; break;
                case  3: outColour += vec4(0.15, 0.25, 0.40, 0.0); break;
            };
        }
#       endif
        return outColour;
    }
#endif

    return vec4(colour, albedo.a);
    
}

vec4 getPixelColour(inout vec3 normal) {
    mat4 colourMatrix = dvd_Matrices[VAR.dvd_baseInstance]._colourMatrix;
    return getPixelColour(getAlbedo(colourMatrix), colourMatrix, normal);
}

vec4 getPixelColour(in vec4 albedo, inout vec3 normal) {
    mat4 colourMatrix = dvd_Matrices[VAR.dvd_baseInstance]._colourMatrix;
    return getPixelColour(albedo, colourMatrix, normal);
}

#endif