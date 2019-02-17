#ifndef _BRDF_FRAG_
#define _BRDF_FRAG_

#if defined(COMPUTE_TBN)
#include "bumpMapping.frag"
#else
#include "lightInput.cmn"
#endif

vec3 private_normalWV = vec3(0.0);

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
#   define getBRDFFactors(A, B, C, D) vec4(0.6, 0.2, 0.9, 0.0); //obvious pink
#else
#   define getBRDFFactors(A, B, C, D) vec4(0.6, 1.0, 0.7, 0.0); //obvious lime-green
#endif
#endif

vec4 getLitColour(in vec4 albedo, in mat4 colourMatrix, in vec3 normal) {
#   if defined(USE_SHADING_FLAT)
    return albedo;
#   endif
    private_normalWV = normal;

    const uint dirLightCount = dvd_LightData.x;
    const vec4 specular = vec4(getSpecular(colourMatrix), getReflectivity(colourMatrix));

    //.a = reflectionCoeff
    vec4 lightColour = vec4(0.0);

    // Apply all lighting contributions
    // Directional lights
    for (uint lightIdx = 0; lightIdx < dirLightCount; ++lightIdx) {
        const Light light = dvd_LightSource[lightIdx];
        lightColour += getBRDFFactors(vec4(light._colour.rgb, 1.0), -light._directionWV.xyz, albedo.rgb, specular);
    }

    //if (dvd_lodLevel < 2) 
    {
        // Point lights

        uint nIndex = GetTileIndex(gl_FragCoord.xy) * LIGHT_NUM_LIGHTS_PER_TILE;

        uint nNextLightIndex = perTileLightIndices[nIndex];
        while (nNextLightIndex != LIGHT_INDEX_BUFFER_SENTINEL) {
            const Light light = dvd_LightSource[int(nNextLightIndex - 1 + dirLightCount)];
            vec3 lightDirection = light._positionWV.xyz - VAR._vertexWV.xyz;

            vec4 colourAndAtt = vec4(light._colour.rgb, getLightAttenuationPoint(light, lightDirection));

            lightColour += getBRDFFactors(colourAndAtt, lightDirection, albedo.rgb, specular);
            nNextLightIndex = perTileLightIndices[++nIndex];
        }

        // Spot lights
        const uint offset = dirLightCount + dvd_LightData.y;
        // Moves past the first sentinel to get to the spot lights.
        nNextLightIndex = perTileLightIndices[++nIndex];
        while (nNextLightIndex != LIGHT_INDEX_BUFFER_SENTINEL) {
            const Light light = dvd_LightSource[int(nNextLightIndex - 1 + offset)];
            vec3 lightDirection = light._positionWV.xyz - VAR._vertexWV.xyz;
            vec4 colourAndAtt = vec4(light._colour.rgb, getLightAttenuationSpot(light, lightDirection));

            lightColour += getBRDFFactors(colourAndAtt, lightDirection, albedo.rgb, specular);
            nNextLightIndex = perTileLightIndices[++nIndex];
        }

    }
    vec3 colour = mix(getEmissive(colourMatrix), lightColour.rgb, DIST_TO_ZERO(length(lightColour.rgb)));

#if defined(IS_REFLECTIVE)
    /*if (dvd_lodLevel < 1) {
        vec3 reflectDirection = reflect(normalize(VAR._vertexWV.xyz), processedNormal);
        reflectDirection = vec3(inverse(dvd_ViewMatrix) * vec4(reflectDirection, 0.0));
        colour = mix(texture(texEnvironmentCube, vec4(reflectDirection, dvd_reflectionIndex)).rgb,
                    colour,
                    vec3(saturate(lightColour.a)));
    }*/
#endif

    return vec4(colour, albedo.a);
}

vec4 getPixelColour(in vec4 albedo, in mat4 colourMatrix, in vec3 normal) {
    vec4 colour = getLitColour(albedo, colourMatrix, normal);

    // Apply shadowing
#if !defined(DISABLE_SHADOW_MAPPING)
    colour.rgb *= getShadowFactor();
#   if defined(DEBUG_SHADOWMAPPING)
    if (dvd_showDebugInfo) {
        switch (g_shadowTempInt) {
        case -1: colour.rgb = vec3(1.0); break;
        case  0: colour.r += 0.15; break;
        case  1: colour.g += 0.25; break;
        case  2: colour.b += 0.40; break;
        case  3: colour.rgb += vec3(0.15, 0.25, 0.40); break;
        };
    }
#   endif
#endif
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
#   endif
    }
#endif

#   if defined (USE_DOUBLE_SIDED)
    if (!gl_FrontFacing) {
        normal = -normal;
    }
#   endif

    return normal;
}

#endif