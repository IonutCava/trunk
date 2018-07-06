#ifndef _BRDF_FRAG_
#define _BRDF_FRAG_

#include "lightInput.cmn"
#include "lightData.frag"
#include "materialData.frag"
#include "shadowMapping.frag"
#include "pbr.frag"
#include "phong_lighting.frag"

void
getBRDFFactors(in int lightIndex,
               in vec3 normalWV,
               in vec3 albedo,
               in vec3 specular,
               in float reflectivity, //PBR: roughness. Phong: shininess
               inout vec3 colourInOut,
               inout float reflectionCoeff)
{
#if defined(USE_SHADING_PHONG) || defined (USE_SHADING_BLINN_PHONG)
    Phong(lightIndex, normalWV, albedo, specular, reflectivity, colourInOut, reflectionCoeff);
#elif defined(USE_SHADING_TOON)
    // ToDo
#else //if defined(USE_SHADING_COOK_TORRANCE) || defined(USE_SHADING_OREN_NAYAR)
    PBR(lightIndex, normalWV, albedo, specular, reflectivity, colourInOut, reflectionCoeff);
#endif
}

uint GetNumLightsInThisTile(uint nTileIndex)
{
    uint nNumLightsInThisTile = 0;
    uint nIndex = uint(dvd_otherData.w) * nTileIndex;
    uint nNextLightIndex = perTileLightIndices[nIndex];
    while (nNextLightIndex != LIGHT_INDEX_BUFFER_SENTINEL)
    {
        nIndex++;
        nNextLightIndex = perTileLightIndices[nIndex];
        nNumLightsInThisTile++;
    }

    return nNumLightsInThisTile;
}

bool isReflective(float specularCoeff) {
    return specularCoeff > 0.75 && dvd_lodLevel(VAR.dvd_drawID) < 1;
}

vec4 getPixelColour(const in vec2 texCoord) {
    //Occlusion culling visibility debug code
#if defined(USE_HIZ_CULLING) && defined(DEBUG_HIZ_CULLING)
    if (dvd_customData > 2.0) {
        return vec4(1.0, 0.0, 0.0, 1.0);
    }
#endif
    vec4 albedo = getAlbedo();
    vec3 processedNormal = getProcessedNormal();

#   if defined (USE_DOUBLE_SIDED)
        processedNormal = gl_FrontFacing ? processedNormal : -processedNormal;
#   endif

    float reflectionCoeff = 0.0;
#   if defined(USE_SHADING_FLAT)
        vec3 colour = albedo.rgb;
#   else
        vec3 specular = getSpecular();
        float reflectivity = getReflectivity();
        vec3 lightColour = vec3(0.0);
        // Apply all lighting contributions
        uint lightIdx;
        // Directional lights
        for (lightIdx = 0; lightIdx < dvd_lightCountPerType[0]; ++lightIdx) {
            getBRDFFactors(int(lightIdx), processedNormal, albedo.rgb, specular, reflectivity, lightColour, reflectionCoeff);
        }

        uint offset = dvd_lightCountPerType[0];
        // Point lights
        uint nIndex = uint(dvd_otherData.w) * GetTileIndex(gl_FragCoord.xy);
        uint nNextLightIndex = perTileLightIndices[nIndex];
        while (nNextLightIndex != LIGHT_INDEX_BUFFER_SENTINEL)
        {
            uint nLightIndex = nNextLightIndex;
            nNextLightIndex = perTileLightIndices[++nIndex];
            getBRDFFactors(int(nLightIndex - 1 + offset), processedNormal, albedo.rgb, specular, reflectivity, lightColour, reflectionCoeff);
        }

        offset = dvd_lightCountPerType[1];
        // Spot lights
        // Moves past the first sentinel to get to the spot lights.
        nNextLightIndex = perTileLightIndices[++nIndex];
        while (nNextLightIndex != LIGHT_INDEX_BUFFER_SENTINEL)
        {
            uint nLightIndex = nNextLightIndex;
            nNextLightIndex = perTileLightIndices[++nIndex];
            getBRDFFactors(int(nLightIndex - 1 + offset), processedNormal, albedo.rgb, specular, reflectivity, lightColour, reflectionCoeff);
        }
    
        vec3 colour = mix(getEmissive(), lightColour, DIST_TO_ZERO(length(lightColour)));
#endif

    if (isReflective(reflectionCoeff)) {
        vec3 reflectDirection = reflect(normalize(VAR._vertexWV.xyz), processedNormal);
        reflectDirection = vec3(inverse(dvd_ViewMatrix) * vec4(reflectDirection, 0.0));
        colour = mix(texture(texEnvironmentCube, vec4(reflectDirection, dvd_reflectionIndex)).rgb,
                    colour,
                    vec3(reflectionCoeff));

    }

    colour *= mix(mix(1.0, 2.0, dvd_isHighlighted), 3.0, dvd_isSelected);
    // Apply shadowing
    colour *= mix(1.0, shadow_loop(), dvd_shadowMapping);

#if defined(DEBUG_SHADOWMAPPING)
    if (dvd_showDebugInfo == 1) {
        switch (g_shadowTempInt){
#if defined(DEBUG_SHADOWSPLITS)
            case -1: colour = vec3(1.0); break;
            case  0: colour = vec3(1.0, 0.0, 0.0); break;
            case  1: colour = vec3(0.0, 1.0, 0.0); break;
            case  2: colour = vec3(0.0, 0.0, 1.0); break;
            case  3: colour = vec3(1.0, 0.25, 0.40); break;
#else
            case -1: colour = vec3(1.0); break;
            case  0: colour.r += 0.15; break;
            case  1: colour.g += 0.25; break;
            case  2: colour.b += 0.40; break;
            case  3: colour += vec3(0.15, 0.25, 0.40); break;
#endif
        };
    }
#endif

    return vec4(colour, albedo.a);
}

vec4 getPixelColour(const in vec2 texCoord, const in vec3 normalWV) {
    setProcessedNormal(normalWV);
    return getPixelColour(texCoord);
}

#endif