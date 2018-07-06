#ifndef _BRDF_FRAG_
#define _BRDF_FRAG_

#include "utility.frag"
#include "lightInput.cmn"
#include "lightData.frag"
#include "materialData.frag"
#include "shadowMapping.frag"
#include "phong_lighting.frag"

vec3 processedNormal = vec3(0.0, 0.0, 1.0);

//TEMP PBR
/// Smith GGX Visibility
///     nDotL: dot-prod of surface normal and light direction
///     nDotV: dot-prod of surface normal and view direction
///     roughness: surface roughness
float SmithGGXVisibility(in float nDotL, in float nDotV, in float roughness)
{
    float rough2 = roughness * roughness;
    float gSmithV = nDotV + sqrt(nDotV * (nDotV - nDotV * rough2) + rough2);
    float gSmithL = nDotL + sqrt(nDotL * (nDotL - nDotL * rough2) + rough2);
    return 1.0 / (gSmithV * gSmithL);
}


float SchlickG1(in float factor, in float rough2)
{
    return 1.0 / (factor * (1.0 - rough2) + rough2);
}

/// Schlick approximation of Smith GGX
///     nDotL: dot product of surface normal and light direction
///     nDotV: dot product of surface normal and view direction
///     roughness: surface roughness
float SchlickVisibility(float nDotL, float nDotV, float roughness)
{
    const float rough2 = roughness * roughness;
    return (SchlickG1(nDotL, rough2) * SchlickG1(nDotV, rough2)) * 0.25;
}

void PBR(in int lightIndex, in vec3 normalWV, inout vec4 colourInOut) {
    vec3 lightDirection = getLightDirection(lightIndex);
    vec3 lightDir = normalize(lightDirection);
    float roughness = dvd_MatShininess;


    colourInOut = vec4(1.0);
}

//TEMP PBR
void getBRDFFactors(in int lightIndex,
                    in vec3 normalWV,
                    inout vec3 colourInOut)
{
#if defined(USE_SHADING_PHONG) || defined (USE_SHADING_BLINN_PHONG)
    Phong(lightIndex, normalWV, colourInOut);
#elif defined(USE_SHADING_TOON)
#elif defined(USE_SHADING_OREN_NAYAR)
#else //if defined(USE_SHADING_COOK_TORRANCE)
    PBR(lightIndex, normalWV, colourInOut);
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

vec4 getPixelColour(const in vec2 texCoord, in vec3 normalWV) {
    //Occlusion culling visibility debug code
#if defined(USE_HIZ_CULLING) && defined(DEBUG_HIZ_CULLING)
    if (dvd_customData > 2.0) {
        return vec4(1.0, 0.0, 0.0, 1.0);
    }
#endif

    parseMaterial();

    processedNormal = normalWV;

#if defined(HAS_TRANSPARENCY)
#   if defined(USE_OPACITY_DIFFUSE)
        float alpha = dvd_MatDiffuse.a;
#   endif
#   if defined(USE_OPACITY_MAP)
        vec4 opacityMap = texture(texOpacityMap, texCoord);
        float alpha = max(min(opacityMap.r, opacityMap.g), min(opacityMap.b, opacityMap.a));
#   endif
#   else
        const float alpha = 1.0;
#   endif

#   if defined (USE_DOUBLE_SIDED)
        processedNormal = gl_FrontFacing ? processedNormal : -processedNormal;
#   endif

#   if defined(USE_SHADING_FLAT)
        vec3 colour = dvd_MatDiffuse.rgb;
#   else
    vec3 lightColour = vec3(0.0);
    // Apply all lighting contributions
    uint lightIdx;
    // Directional lights
    for (lightIdx = 0; lightIdx < dvd_lightCountPerType[0]; ++lightIdx) {
        getBRDFFactors(int(lightIdx), processedNormal, lightColour);
    }

    uint offset = dvd_lightCountPerType[0];
    // Point lights
    uint nIndex = uint(dvd_otherData.w) * GetTileIndex(gl_FragCoord.xy);
    uint nNextLightIndex = perTileLightIndices[nIndex];
    while (nNextLightIndex != LIGHT_INDEX_BUFFER_SENTINEL)
    {
        uint nLightIndex = nNextLightIndex;
        nNextLightIndex = perTileLightIndices[++nIndex];

        getBRDFFactors(int(nLightIndex - 1 + offset), processedNormal, lightColour);
    }

    offset = dvd_lightCountPerType[1];
    // Spot lights
    // Moves past the first sentinel to get to the spot lights.
    nNextLightIndex = perTileLightIndices[++nIndex];
    while (nNextLightIndex != LIGHT_INDEX_BUFFER_SENTINEL)
    {
        uint nLightIndex = nNextLightIndex;
        nNextLightIndex = perTileLightIndices[++nIndex];
        getBRDFFactors(int(nLightIndex - 1 + offset), processedNormal, lightColour);
    }
    
    vec3 colour = mix(dvd_MatEmissive, lightColour, DIST_TO_ZERO(length(lightColour)));
#endif

    float reflectance = saturate(dvd_MatShininess / 255.0);
    if (reflectance > 0.75 && dvd_lodLevel < 1) {
        vec3 reflectDirection = reflect(normalize(VAR._vertexWV.xyz), processedNormal);
        reflectDirection = vec3(inverse(dvd_ViewMatrix) * vec4(reflectDirection, 0.0));
        colour = mix(texture(texEnvironmentCube, vec4(reflectDirection, 0.0)).rgb,
                    colour,
                    vec3(reflectance));

    }

    colour *= mix(mix(1.0, 2.0, dvd_isHighlighted), 3.0, dvd_isSelected);
    // Apply shadowing
    colour *= mix(1.0, shadow_loop(), dvd_shadowMapping);

#if defined(_DEBUG) && defined(DEBUG_SHADOWMAPPING)
    if (dvd_showDebugInfo == 1) {
        switch (_shadowTempInt){
            case -1: colour    = vec3(1.0); break;
            case  0: colour.r += 0.15; break;
            case  1: colour.g += 0.25; break;
            case  2: colour.b += 0.40; break;
            case  3: colour   += vec3(0.15, 0.25, 0.40); break;
        };
    }
#endif
    return vec4(colour, alpha);
}

#endif