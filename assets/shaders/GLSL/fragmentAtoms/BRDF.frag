#ifndef _BRDF_FRAG_
#define _BRDF_FRAG_

#include "lightInput.cmn"
#include "lightData.frag"
#include "utility.frag"
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
    uint lightsPerTile = uint(dvd_otherData.w);
    uint offset = lightsPerTile * nTileIndex;
    // count point lights
    for (uint i = 0; i < lightsPerTile; ++i) {
        uint light = perTileLightIndices[offset + i];
        if (light > 0 && light < LIGHT_INDEX_BUFFER_SENTINEL) {
            nNumLightsInThisTile++;
        } else if (light == LIGHT_INDEX_BUFFER_SENTINEL) {
            break;
        }
    }

    return nNumLightsInThisTile;
}

vec4 getPixelColor(const in vec2 texCoord, in vec3 normalWV) {
    if (dvd_lodLevel > 2 && (texCoord.x + texCoord.y == 0)) {
        discard;
    }
    
    parseMaterial();

#if defined(HAS_TRANSPARENCY)
#   if defined(USE_OPACITY_DIFFUSE)
        float alpha = dvd_MatDiffuse.a;
#   endif
#   if defined(USE_OPACITY_MAP)
        vec4 opacityMap = texture(texOpacityMap, texCoord);
        float alpha = max(min(opacityMap.r, opacityMap.g), min(opacityMap.b, opacityMap.a));
#   endif
        if (dvd_useAlphaTest && alpha < ALPHA_DISCARD_THRESHOLD){
            discard;
        }
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
    for (lightIdx = 0; lightIdx < VAR._lightCount[0]; ++lightIdx) {
        getBRDFFactors(int(lightIdx), normalWV, lightColor);
    }
    uint offset = lightIdx + 1;
    // Point lights
    for (lightIdx = 0; lightIdx < VAR._lightCount[1]; ++lightIdx) {
        getBRDFFactors(int(lightIdx + offset), normalWV, lightColor);
    }
    offset += lightIdx + 1;
    // Spot lights
    for (lightIdx = 0; lightIdx < VAR._lightCount[2]; ++lightIdx) {
        getBRDFFactors(int(lightIdx + offset), normalWV, lightColor);
    }

    /*uint nTileIndex = GetTileIndex(gl_FragCoord.xy);
    uint nIndex = uint(dvd_otherData.w) * nTileIndex;
    uint nNextLightIndex = perTileLightIndices[nIndex];
    uint count = 0;
    while (nNextLightIndex != LIGHT_INDEX_BUFFER_SENTINEL)
    {
        uint nLightIndex = nNextLightIndex;
        nNextLightIndex = perTileLightIndices[++nIndex];
        // offset index to account for directional lights
        //getBRDFFactors(int(nLightIndex + offset), normalWV, lightColor);
        if (nLightIndex > 0) {
            count++;
            if (count > 10) {
                break;
            }
        }
    }
    return vec4(0.2, 0.2, 0.2, 1.0) + (float(GetTileIndex(gl_FragCoord.xy)) / 5100.0);
    uint tileCount = GetNumLightsInThisTile(GetTileIndex(gl_FragCoord.xy));
    if (tileCount == 0) {
        return vec4(0.0, 0.0, 0.0, 1.0);
    } else if (tileCount == 1) {
        return vec4(0.5, 0.5, 0.5, 1.0);
    } else {
        return vec4(1.0);
    }
    // Moves past the first sentinel to get to the spot lights.
    nIndex++;
    nNextLightIndex = perTileLightIndices[nIndex];

    // Loops over spot lights.
    while (nNextLightIndex != LIGHT_INDEX_BUFFER_SENTINEL)
    {
    uint nLightIndex = nNextLightIndex;
    nIndex++;
    nNextLightIndex = perTileLightIndices[nIndex];
    // offset index to account for directional and point lights
    uint lightIndex = nNextLightIndex + dvd_lightCountPerType[0] + dvd_lightCountPerType[1];
    }
*/

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