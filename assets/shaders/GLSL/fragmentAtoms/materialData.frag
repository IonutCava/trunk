#ifndef _MATERIAL_DATA_FRAG_
#define _MATERIAL_DATA_FRAG_

#include "nodeBufferedInput.cmn"
#include "utility.frag"

//Ref: https://github.com/urho3d/Urho3D/blob/master/bin/CoreData/Shaders/GLSL/PBRLitSolid.glsl
#if defined(USE_ALBEDO_ALPHA) || defined(USE_OPACITY_MAP)
#   define HAS_TRANSPARENCY
#endif

#if !defined(PRE_PASS) || defined(HAS_TRANSPARENCY)
layout(binding = TEXTURE_UNIT0) uniform sampler2D texDiffuse0;
layout(binding = TEXTURE_UNIT1) uniform sampler2D texDiffuse1;
#endif

#if !defined(PRE_PASS)

#if defined(USE_PLANAR_REFRACTION)
layout(binding = TEXTURE_REFRACTION_PLANAR) uniform sampler2D texRefractPlanar;
#endif //USE_PLANAR_REFRACTION


#if defined(USE_OCCLUSION_METALLIC_ROUGHNESS_MAP)
layout(binding = TEXTURE_OCCLUSION_METALLIC_ROUGHNESS) uniform sampler2D texOcclusionMetallicRoughness;
#endif //USE_METALLIC_ROUGHNESS_MAP

#if defined(USE_SSAO)
layout(binding = TEXTURE_GBUFFER_EXTRA)  uniform sampler2D texGBufferExtra;
#endif //USE_SSAO

#endif  //PRE_PASS

//Specular and opacity maps are available even for non-textured geometry
#if defined(USE_OPACITY_MAP)
layout(binding = TEXTURE_OPACITY) uniform sampler2D texOpacityMap;
#endif

#if !defined(USE_CUSTOM_NORMAL_MAP)
//Normal or BumpMap
layout(binding = TEXTURE_NORMALMAP) uniform sampler2D texNormalMap;
#endif

#if defined(COMPUTE_TBN)
#include "bumpMapping.frag"
#endif

#define TexCoords VAR._texCoord

float getDisplacementValue(vec2 sampleUV);
vec2 ParallaxOcclusionMapping(vec2 sampleUV, vec3 viewDir, float currentDepthMapValue)
{
    // number of depth layers
    const float minLayers = 8.0;
    const float maxLayers = 32.0;
    float numLayers = mix(maxLayers, minLayers, abs(dot(vec3(0.0, 0.0, 1.0), viewDir)));
    // calculate the size of each layer
    float layerDepth = 1.0 / numLayers;
    // depth of current layer
    float currentLayerDepth = 0.0;
    // the amount to shift the texture coordinates per layer (from vector P)
    vec2 P = viewDir.xy * dvd_parallaxFactor;
    vec2 deltaTexCoords = P / numLayers;

    // get initial values
    vec2  currentTexCoords = sampleUV;
    while (currentLayerDepth < currentDepthMapValue)
    {
        // shift texture coordinates along direction of P
        currentTexCoords -= deltaTexCoords;
        // get depthmap value at current texture coordinates
        currentDepthMapValue = getDisplacementValue(currentTexCoords);
        // get depth of next layer
        currentLayerDepth += layerDepth;
    }

    const vec2 prevTexCoords = currentTexCoords + deltaTexCoords;

    // get depth after and before collision for linear interpolation
    const float afterDepth = currentDepthMapValue - currentLayerDepth;
    const float beforeDepth = getDisplacementValue(prevTexCoords) - currentLayerDepth + layerDepth;

    // interpolation of texture coordinates
    const float weight = afterDepth / (afterDepth - beforeDepth);
    return prevTexCoords * weight + currentTexCoords * (1.0f - weight);
}

#if !defined(PRE_PASS)
float Gloss(in vec3 bump, in vec2 uv)
{
    #if defined(USE_TOKSVIG)
        // Compute the "Toksvig Factor"
        float rlen = 1.0/saturate(length(bump));
        return 1.0/(1.0 + power*(rlen - 1.0));
    #elif defined(USE_TOKSVIG_MAP)
        float baked_power = 100.0;
        // Fetch pre-computed "Toksvig Factor"
        // and adjust for specified power
        float gloss = texture2D(texOcclusionMetallicRoughness, uv).a;
        gloss /= mix(power/baked_power, 1.0, gloss);
        return gloss;
    #else
        return 1.0;
    #endif
}

float getSSAO() {
#if defined(USE_SSAO)
    return texture(texGBufferExtra, dvd_screenPositionNormalised).b;
#else
    return 1.0f;
#endif
}

#if defined(USE_CUSTOM_ROUGHNESS)
vec3 getOcclusionMetallicRoughness(in mat4 colourMatrix, in vec2 uv);
#else //USE_CUSTOM_ROUGHNESS
vec3 getOcclusionMetallicRoughness(in mat4 colourMatrix, in vec2 uv) {
#if defined(USE_OCCLUSION_METALLIC_ROUGHNESS_MAP)
    vec3 omr = texture(texOcclusionMetallicRoughness, uv).rgb;
#if defined(PBR_SHADING)
    return omr;
#else
    // Convert specular to roughness ... roughly? really wrong, but should work I guess. -Ionutsssssssss
    return vec3(Occlusion(colourMatrix), Metallic(colourMatrix), 1.0f - omr.r);
#endif
#else
    return vec3(Occlusion(colourMatrix), Metallic(colourMatrix), Roughness(colourMatrix));
#endif
}
#endif //USE_CUSTOM_ROUGHNESS

vec3 getEmissive(in mat4 colourMatrix) {
    return EmissiveColour(colourMatrix);
}

void setEmissive(in mat4 colourMatrix, in vec3 value) {
    EmissiveColour(colourMatrix) = value;
}

#endif //PRE_PASS
float getOpacity(in mat4 colourMatrix, in float albedoAlpha, in vec2 uv) {
#if defined(HAS_TRANSPARENCY)
#   if defined(USE_OPACITY_MAP)
#       if defined(USE_OPACITY_MAP_RED_CHANNEL)
            return texture(texOpacityMap, uv).r;
#       else
            return texture(texOpacityMap, uv).a;
#       endif
#   else
        return albedoAlpha;
#   endif
#else
    return 1.0;
#endif
}

#if !defined(PRE_PASS) || defined(HAS_TRANSPARENCY)
vec4 getTextureColour(in vec4 albedo, in vec2 uv) {
#define TEX_NONE 0
#define TEX_MODULATE 1
#define TEX_ADD  2
#define TEX_SUBTRACT  3
#define TEX_DIVIDE  4
#define TEX_SMOOTH_ADD  5
#define TEX_SIGNED_ADD  6
#define TEX_DECAL  7
#define TEX_REPLACE  8

#if defined(SKIP_TEX0)
    if (dvd_texOperation != TEX_NONE) {
        return texture(texDiffuse1, uv);
    }
        
    return albedo;
#else
    vec4 colour = texture(texDiffuse0, uv);
    // Read from the second texture (if any)
    switch (dvd_texOperation) {
        default:
            //hot pink to easily spot it in a crowd
            colour = vec4(0.7743, 0.3188, 0.5465, 1.0);
            break;
        case TEX_NONE:
            //NOP
            break;
        case TEX_MODULATE:
            colour *= texture(texDiffuse1, uv);
            break;
        case TEX_REPLACE:
            colour = texture(texDiffuse1, uv);
            break;
        case TEX_SIGNED_ADD:
            colour += texture(texDiffuse1, uv) - 0.5f;
            break;
        case TEX_DIVIDE:
            colour /= texture(texDiffuse1, uv);
            break;
        case TEX_SUBTRACT:
            colour -= texture(texDiffuse1, uv);
            break;
        case TEX_DECAL: {
            vec4 colour2 = texture(texDiffuse1, uv);
            colour = vec4(mix(colour.rgb, colour2.rgb, colour2.a), colour.a);
        } break;
        case TEX_ADD: {
            vec4 colour2 = texture(texDiffuse1, uv);
            colour.rgb += colour2.rgb;
            colour.a *= colour2.a;
        }break;
        case TEX_SMOOTH_ADD: {
            vec4 colour2 = texture(texDiffuse1, uv);
            colour = (colour + colour2) - (colour * colour2);
        }break;
    }

    return saturate(colour);
#endif
}

vec4 getAlbedo(in mat4 colourMatrix, in vec2 uv) {
    vec4 albedo = getTextureColour(BaseColour(colourMatrix), uv);
    albedo.a = getOpacity(colourMatrix, albedo.a, uv);

    return albedo;
}
#endif //!defined(PRE_PASS) || defined(HAS_TRANSPARENCY)

// Computed normals are NOT normalized. Retrieved normals ARE.
vec3 getNormal(in vec2 uv) {
#if defined(PRE_PASS) || !defined(USE_DEFERRED_NORMALS)
    vec3 normal = VAR._normalWV;

#   if defined(COMPUTE_TBN) && !defined(USE_CUSTOM_NORMAL_MAP)
        if (dvd_bumpMethod != BUMP_NONE) {
            normal = VAR._tbn * getBump(uv);
        }
#   endif //COMPUTE_TBN

#   if defined (USE_DOUBLE_SIDED)
        if (!gl_FrontFacing) {
            normal = -normal;
        }
#   endif //USE_DOUBLE_SIDED

    return normal;
#else //PRE_PASS
    return normalize(unpackNormal(texture(texNormalMap, dvd_screenPositionNormalised).rg));
#endif //PRE_PASS
}
#endif //_MATERIAL_DATA_FRAG_