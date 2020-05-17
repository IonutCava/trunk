#ifndef _BRDF_FRAG_
#define _BRDF_FRAG_

#include "utility.cmn"
#include "lightInput.cmn"

#include "materialData.frag"
#include "shadowMapping.frag"

#define M_PI 3.14159265358979323846
#define M_EPSILON 0.0000001
#define MIN_SPEC_REFLECTION 0.33f

#if defined(USE_SHADING_COOK_TORRANCE) || defined(USE_SHADING_OREN_NAYAR)
#include "pbr.frag"
vec4 getExtraData(in mat4 colourMatrix, in vec2 uv) {
    return vec4(getMetallicRoughness(colourMatrix, uv), getRimLighting(colourMatrix, uv), 0.0f);
}
#define getBRDFFactors(LDir, LCol, MetallicRoughness, Albedo, N, NdotL) PBR(LDir, LCol, MetallicRoughness, Albedo, N, NdotL);
#elif defined(USE_SHADING_PHONG) || defined (USE_SHADING_BLINN_PHONG)
#include "phong_lighting.frag"
vec4 getExtraData(in mat4 colourMatrix, in vec2 uv) {
    return vec4(getSpecular(colourMatrix, uv), getShininess(colourMatrix));
}
#define getBRDFFactors(LDir, LCol, SpecularShininess, Albedo, N, NdotL) Phong(normalize(LDir), LCol, SpecularShininess, Albedo, N, NdotL)
#else
#if defined(USE_SHADING_TOON) // ToDo
#   define getBRDFFactors(LDir, LCol, Data, Albedo, N, NdotL) vec4(0.6f, 0.2f, 0.9f, 0.0f); //obvious pink
#else
#   define getBRDFFactors(LDir, LCol, Data, Albedo, N, NdotL) vec4(0.6f, 1.0f, 0.7f, 0.0f); //obvious lime-green
#endif
#endif

#if !defined(MAX_LIGHTS_PER_PASS)
#define MAX_LIGHTS_PER_PASS MAX_LIGHTS_PER_TILE
#endif

uint GetTileIndex() {
    ivec2 tileID = ivec2(gl_FragCoord.xy) / ivec2(FORWARD_PLUS_TILE_RES, FORWARD_PLUS_TILE_RES);
    return uint(tileID.y * dvd_numTilesX + tileID.x);
}

vec4 getDirectionalLightContribution(in uint dirLightCount, in vec3 albedo, in vec4 data, in vec3 normalWV) {
    vec4 ret = vec4(0.0);
    for (uint lightIdx = 0; lightIdx < dirLightCount; ++lightIdx) {
        const Light light = dvd_LightSource[lightIdx];

        const vec3 lightDirectionWV = -light._directionWV.xyz;
        const float ndl = saturate((dot(normalWV, normalize(lightDirectionWV))));
        const float shadowFactor = getShadowFactorDirectional(light._options.y, tan(acos(ndl)));

        vec4 temp = getBRDFFactors(lightDirectionWV,
                                   vec4(light._colour.rgb, 1.0f),
                                   data,
                                   vec4(albedo, shadowFactor), 
                                   normalWV,
                                   ndl);

        ret.rgb += temp.rgb;
        ret.a = max(ret.a, temp.a);
    }

    return ret;
}

float getPointAttenuation(in Light light, in vec3 lightDirectionWV) {
    const float radius = light._positionWV.w;
    const float dist = length(lightDirectionWV); 
    const float att = saturate(1.0f - ((dist * dist) / (radius * radius)));
    return att * att;
}

float getSpotAttenuation(in Light light, in vec3 lightDirectionWV) {
    const vec3 spotDirectionWV = normalize(light._directionWV.xyz);
    const float cosOuterConeAngle = light._colour.w;
    const float cosInnerConeAngle = light._directionWV.w;

    const float theta = dot(normalize(lightDirectionWV), normalize(-spotDirectionWV));
    const float intensity = saturate((theta - cosOuterConeAngle) / (cosInnerConeAngle - cosOuterConeAngle));

    const float radiusSpot = mix(float(light._options.z), light._positionWV.w, 1.0f - intensity);

    const float dist = length(lightDirectionWV);
    const float att = saturate(1.0f - ((dist * dist) / (radiusSpot * radiusSpot)));
    return att * intensity;
}

vec4 getOtherLightContribution(in uint dirLightCount, in vec3 albedo, in vec4 data, in vec3 normalWV) {
    const uint offset = GetTileIndex() * MAX_LIGHTS_PER_TILE;

    float shadowFactor = 1.0f;
    vec4 ret = vec4(0.0);
    for (uint i = 0; i < MAX_LIGHTS_PER_PASS; ++i) {
        const int lightIdx = perTileLightIndices[offset + i];
        if (lightIdx == -1) {
            break;
        }

        const Light light = dvd_LightSource[lightIdx + dirLightCount];
        const vec3 lightDirectionWV = light._positionWV.xyz - VAR._vertexWV.xyz;
        const float ndl = saturate((dot(normalWV, normalize(lightDirectionWV))));

        float att = 0.0f;
        if (light._options.x == 1) {
            att = getPointAttenuation(light, lightDirectionWV);
            shadowFactor = getShadowFactorPoint(light._options.y, tan(acos(ndl)));
        } else {
            att = getSpotAttenuation(light, lightDirectionWV);
            shadowFactor = getShadowFactorSpot(light._options.y, tan(acos(ndl)));
        }

        const vec4 colourAndAtt = vec4(light._colour.rgb, att);
        vec4 temp = getBRDFFactors(lightDirectionWV, colourAndAtt, data, vec4(albedo, shadowFactor), normalWV, ndl);
        ret.rgb += temp.rgb;
        ret.a = max(ret.a, temp.a);
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
        ret *= getShadowFactorDirectional(dvd_LightSource[lightIdx]._options.y, tan(acos(ndl)));
    }

    const uint offset = GetTileIndex() * MAX_LIGHTS_PER_TILE;
    for (uint i = 0; i < MAX_LIGHTS_PER_PASS; ++i) {
        const int lightIdx = perTileLightIndices[offset + i];
        if (lightIdx == -1) {
            break;
        }
        const Light light = dvd_LightSource[lightIdx + dirLightCount];
             
        const vec3 lightDirectionWV = light._positionWV.xyz - VAR._vertexWV.xyz;
        const float ndl = saturate((dot(normalWV, normalize(lightDirectionWV))));
        if (light._options.x == 2) {
            ret *= getShadowFactorSpot(light._options.y, tan(acos(ndl)));
        } else {
            ret *= getShadowFactorPoint(light._options.y, tan(acos(ndl)));
        }
    }
    

    return ret;
}

vec3 lightTileColour() {
    ivec2 loc = ivec2(gl_FragCoord.xy);
    ivec2 tileID = loc / ivec2(FORWARD_PLUS_TILE_RES, FORWARD_PLUS_TILE_RES);
    uint index = tileID.y * dvd_numTilesX + tileID.x;
    uint offset = index * MAX_LIGHTS_PER_TILE;

    uint count = 0;
    for (uint i = 0; i < MAX_LIGHTS_PER_TILE; i++) {
        if (perTileLightIndices[offset + i] == -1) {
            break;
        }

        count++;
    }
    float shade = float(count) / float(MAX_LIGHTS_PER_TILE * 2);
    return vec3(shade);
}

vec3 getLitColour(in vec3 albedo, in mat4 colourMatrix, in vec3 normalWV, in vec2 uv) {
    switch (dvd_materialDebugFlag) {
        case DEBUG_ALBEDO: return albedo;
        case DEBUG_SPECULAR: return getSpecular(colourMatrix, uv);
        case DEBUG_EMISSIVE: return getEmissive(colourMatrix);
        case DEBUG_ROUGHNESS: return vec3(getMetallicRoughness(colourMatrix, uv).g);
        case DEBUG_METALLIC: return vec3(getMetallicRoughness(colourMatrix, uv).r);
        case DEBUG_NORMALS: return (inverse(dvd_ViewMatrix) * vec4(normalWV, 0)).xyz;
        case DEBUG_SHADOW_MAPS: return vec3(getShadowFactor(normalWV));
        case DEBUG_LIGHT_TILES: return lightTileColour();
    }

#if defined(USE_SHADING_FLAT)
    return albedo;
#else //USE_SHADING_FLAT
    const vec4 data = getExtraData(colourMatrix, uv);

    //albedo.rgb += dvd_AmbientColour.rgb;
    albedo.rgb *= getSSAO();

    // Apply all lighting contributions (.a = reflectionCoeff)
    const uint dirLightCount = dvd_LightData.x;
    vec4 lightColour1 = getDirectionalLightContribution(dirLightCount, albedo, data, normalWV);
    vec4 lightColour2 = getOtherLightContribution(dirLightCount, albedo, data, normalWV);
    
    vec3 lightColour = getEmissive(colourMatrix) + lightColour1.rgb + lightColour2.rgb;
    const float reflectivity = saturate(max(lightColour1.a, lightColour2.a));

    // Specular reflections
    if (reflectivity > MIN_SPEC_REFLECTION) {
        /*if (dvd_lodLevel < 1 && getReflectivity(colourMatrix) > 100) {
            vec3 reflectDirection = reflect(VAR._viewDirectionWV, normalWV);
            reflectDirection = vec3(inverse(dvd_ViewMatrix) * vec4(reflectDirection, 0.0f));
            //lightColour.rgb = mix(texture(texEnvironmentCube, vec4(reflectDirection, dvd_reflectionIndex)).rgb,
            //                  lightColour.rgb,
            //                  vec3(saturate(lightColour.a)));
        }*/
    }
    
    return lightColour;
#endif //USE_SHADING_FLAT
}

vec4 getPixelColour(in vec4 albedo, in mat4 colourMatrix, in vec3 normalWV, in vec2 uv) {
    vec4 colour = vec4(getLitColour(albedo.rgb, colourMatrix, normalWV, uv), albedo.a);

    if (dvd_showDebugInfo) {
#if !defined(DISABLE_SHADOW_MAPPING)
        // CSM Info
        if (dvd_CSMSplitsViewIndex > -1) {
            switch (getCSMSlice(dvd_CSMSplitsViewIndex)) {
                case  0: colour.r += 0.15f; break;
                case  1: colour.g += 0.25f; break;
                case  2: colour.b += 0.40f; break;
                case  3: colour.rgb += 1 * vec3(0.15f, 0.25f, 0.40f); break;
                case  4: colour.rgb += 2 * vec3(0.15f, 0.25f, 0.40f); break;
                case  5: colour.rgb += 3 * vec3(0.15f, 0.25f, 0.40f); break;
            };
        }
#endif //DISABLE_SHADOW_MAPPING
        // Other stuff
    }

    return colour;
}

#endif