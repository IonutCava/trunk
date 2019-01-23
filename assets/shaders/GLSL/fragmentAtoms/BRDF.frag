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

void getBRDFFactors(in int lightIndex,
                    in vec3 normalWV,
                    in vec3 albedo,
                    in vec3 specular,
                    in float reflectivity, //PBR: roughness. Phong: shininess
                    inout vec3 colourInOut,
                    inout float reflectionCoeff)
{

    dvd_private_light = dvd_LightSource[lightIndex];

#if defined(USE_SHADING_PHONG) || defined (USE_SHADING_BLINN_PHONG)
    Phong(normalWV, albedo, specular, reflectivity, colourInOut, reflectionCoeff);
#elif defined(USE_SHADING_TOON)
    // ToDo
    reflectionCoeff = 0;
    colourInOut = vec3(0.6, 0.2, 0.9); //obvious pink
#elif defined(USE_SHADING_COOK_TORRANCE) || defined(USE_SHADING_OREN_NAYAR)
    PBR(normalWV, albedo, specular, reflectivity, colourInOut, reflectionCoeff);
#else
    reflectionCoeff = 0;
    colourInOut = vec3(0.6, 1.0, 0.7); //obvious lime-green
#endif
}

vec3 getLightColour(vec3 albedo, vec3 normal, uint dirLightCount, uint pointLightCount) {
#   if defined(USE_SHADING_FLAT)
    return albedo;
#   else
    float reflectionCoeff = 0.0;
    vec3 lightColour = vec3(0.0);
    vec3 specular = getSpecular();
    float reflectivity = getReflectivity();
    // Apply all lighting contributions

    // Directional lights
    for (int lightIdx = 0; lightIdx < dirLightCount; ++lightIdx) {
        getBRDFFactors(lightIdx, normal, albedo, specular, reflectivity, lightColour, reflectionCoeff);
    }

    if (dvd_lodLevel < 2) {
        // Point lights

        uint offset = dirLightCount;
        uint nIndex = GetTileIndex(gl_FragCoord.xy) * LIGHT_NUM_LIGHTS_PER_TILE;

        uint nNextLightIndex = perTileLightIndices[nIndex];
        while (nNextLightIndex != LIGHT_INDEX_BUFFER_SENTINEL) {
            getBRDFFactors(int(nNextLightIndex - 1 + offset), normal, albedo, specular, reflectivity, lightColour, reflectionCoeff);
            nNextLightIndex = perTileLightIndices[++nIndex];
        }

        // Spot lights
        offset += pointLightCount;
        // Moves past the first sentinel to get to the spot lights.
        nNextLightIndex = perTileLightIndices[++nIndex];
        while (nNextLightIndex != LIGHT_INDEX_BUFFER_SENTINEL) {
            getBRDFFactors(int(nNextLightIndex - 1 + offset), normal, albedo, specular, reflectivity, lightColour, reflectionCoeff);
            nNextLightIndex = perTileLightIndices[++nIndex];
        }
    }

    return mix(getEmissive(), lightColour, DIST_TO_ZERO(length(lightColour)));
#endif
}


vec4 getPixelColour(vec4 albedo, vec3 normal) {
#if defined(_DEBUG)
    if (dvd_NormalsOnly) {
        return vec4(getProcessedNormal(), 1.0);
    }
#endif

    vec3 colour = getLightColour(albedo.rgb, normal, dvd_LightData.x, dvd_LightData.y);

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
#if !defined(DISABLE_SHADOW_MAPPING) && defined(DEBUG_SHADOWMAPPING)
    if (dvd_showDebugInfo) {
        switch (g_shadowTempInt){
            case -1: colour = vec3(1.0); break;
            case  0: colour.r += 0.15; break;
            case  1: colour.g += 0.25; break;
            case  2: colour.b += 0.40; break;
            case  3: colour += vec3(0.15, 0.25, 0.40); break;
        };
    }
#endif

#if !defined(DISABLE_SHADOW_MAPPING)
    if (dvd_receivesShadows) {
        return vec4(colour * getShadowFactor(), albedo.a);
    }
#endif

    return vec4(colour, albedo.a);
    
}

vec4 getPixelColour() {
    return getPixelColour(getAlbedo(), getProcessedNormal());
}

#endif