#ifndef _BRDF_FRAG_
#define _BRDF_FRAG_

#include "utility.frag"
#include "lightInput.cmn"
#include "lightData.frag"
#include "materialData.frag"
#include "shadowMapping.frag"
#include "phong_lighting.frag"

void getBRDFFactors(in int lightIndex,
                    in vec3 normalWV,
                    inout vec3 colorInOut)
{
#if defined(USE_SHADING_PHONG) || defined (USE_SHADING_BLINN_PHONG)
    Phong(lightIndex, normalWV, colorInOut);
#elif defined(USE_SHADING_TOON)
#elif defined(USE_SHADING_OREN_NAYAR)
#else //if defined(USE_SHADING_COOK_TORRANCE)
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

vec4 getPixelColor(const in vec2 texCoord, in vec3 normalWV) {
    parseMaterial();

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

#if defined (USE_DOUBLE_SIDED)
        normalWV = gl_FrontFacing ? normalWV : -normalWV;
#endif

#if defined(USE_SHADING_FLAT)
    vec3 color = dvd_MatDiffuse.rgb;
#else
    vec3 lightColor = vec3(0.0);
    // Apply all lighting contributions
    uint lightIdx;
    // Directional lights
    for (lightIdx = 0; lightIdx < dvd_lightCountPerType[0]; ++lightIdx) {
        getBRDFFactors(int(lightIdx), normalWV, lightColor);
    }
    uint offset = dvd_lightCountPerType[0];
    // Point lights
    uint nIndex = uint(dvd_otherData.w) * GetTileIndex(gl_FragCoord.xy);
    uint nNextLightIndex = perTileLightIndices[nIndex];
    while (nNextLightIndex != LIGHT_INDEX_BUFFER_SENTINEL)
    {
        uint nLightIndex = nNextLightIndex;
        nNextLightIndex = perTileLightIndices[++nIndex];

        getBRDFFactors(int(nLightIndex - 1 + offset), normalWV, lightColor);
    }

    offset = dvd_lightCountPerType[1];
    // Spot lights
    // Moves past the first sentinel to get to the spot lights.
    nNextLightIndex = perTileLightIndices[++nIndex];
    while (nNextLightIndex != LIGHT_INDEX_BUFFER_SENTINEL)
    {
        uint nLightIndex = nNextLightIndex;
        nNextLightIndex = perTileLightIndices[++nIndex];
        getBRDFFactors(int(nLightIndex - 1 + offset), normalWV, lightColor);
    }
    
    vec3 color = mix(dvd_MatEmissive, lightColor, DIST_TO_ZERO(length(lightColor)));
#endif

#if defined(USE_REFLECTIVE_CUBEMAP)
    vec3 reflectDirection = reflect(normalize(VAR._vertexWV.xyz), normalWV);
    reflectDirection = vec3(inverse(dvd_ViewMatrix) * vec4(reflectDirection, 0.0));
    color = mix(texture(texEnvironmentCube, vec4(reflectDirection, 0.0)).rgb,
                color,
                1.0 - saturate(dvd_MatShininess / 255.0));
#endif

    color *= mix(mix(1.0, 2.0, dvd_isHighlighted), 3.0, dvd_isSelected);
    // Apply shadowing
    color *= mix(1.0, shadow_loop(), dvd_shadowMapping);

#if defined(_DEBUG) && defined(DEBUG_SHADOWMAPPING)
    if (dvd_showDebugInfo == 1) {
        switch (_shadowTempInt){
            case -1: color    = vec3(1.0); break;
            case  0: color.r += 0.15; break;
            case  1: color.g += 0.25; break;
            case  2: color.b += 0.40; break;
            case  3: color   += vec3(0.15, 0.25, 0.40); break;
        };
    }
#endif
    return vec4(color, alpha);
}

#endif