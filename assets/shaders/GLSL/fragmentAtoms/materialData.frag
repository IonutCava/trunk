#ifndef _MATERIAL_DATA_FRAG_
#define _MATERIAL_DATA_FRAG_

#include "nodeBufferedInput.cmn"
#include "utility.frag"
#include "pbr.frag"

#if defined(COMPUTE_TBN)
#include "bumpMapping.frag"
#endif //COMPUTE_TBN

#ifndef F0
#define F0 vec3(0.04f)
#endif //F0

#if defined(USE_CUSTOM_TBN)
mat3 getTBNWV();
#else //USE_CUSTOM_TBN
#if defined(COMPUTE_TBN)
#define getTBNWV() VAR._tbnWV
#else //COMPUTE_TBN
// Default: T - X-axis, B - Z-axis, N - Y-axis
#define getTBNWV() mat3(vec3(1.f, 0.f, 0.f), vec3(0.f, 0.f, 1.f), vec3(1.f, 0.f, 0.f))
#endif //COMPUTE_TBN
#endif //USE_CUSTOM_TBN

#if !defined(PRE_PASS)
// Reduce specular aliasing by producing a modified roughness value
// Tokuyoshi et al. 2019. Improved Geometric Specular Antialiasing.
// http://www.jp.square-enix.com/tech/library/pdf/ImprovedGeometricSpecularAA.pdf
float specularAntiAliasing(vec3 N, float a) {
    // normal-based isotropic filtering
    // this is originally meant for deferred rendering but is a bit simpler to implement than the forward version
    // saves us from calculating uv offsets and sampling textures for every light
    const float SIGMA2 = 0.25; // squared std dev of pixel filter kernel (in pixels)
    const float KAPPA = 0.18; // clamping threshold
    vec3 dndu = dFdx(N);
    vec3 dndv = dFdy(N);
    float variance = SIGMA2 * (dot(dndu, dndu) + dot(dndv, dndv));
    float kernelRoughness2 = min(2.0 * variance, KAPPA);
    return saturate(a + kernelRoughness2);
}

#define SpecularColour(diffColour, metallic) mix(F0, diffColour, metallic)

#define OCCLUSSION(OMR_IN) OMR_IN.r
#define METALLIC(OMR_IN) OMR_IN.g
#define ROUGHNESS(OMR_IN) OMR_IN.b

#if defined(USE_CUSTOM_ROUGHNESS)
vec3 getOcclusionMetallicRoughness(in NodeMaterialData data, in vec2 uv);
#else //USE_CUSTOM_ROUGHNESS
vec3 getOcclusionMetallicRoughness(in NodeMaterialData data, in vec2 uv) {
#if defined(USE_OCCLUSION_METALLIC_ROUGHNESS_MAP)
    vec3 omr = saturate(texture(texOMR, uv).rgb);
#if defined(PBR_SHADING)
    return omr;
#else //PBR_SHADING
    // Convert specular to roughness ... roughly? really wrong, but should work I guesssssssssss. -Ionut
    return saturate(vec3(PACKED_OMR(data).rg, 1.0f - omr[2]));
#endif //PBR_SHADING
#else //USE_OCCLUSION_METALLIC_ROUGHNESS_MAP
    return saturate(PACKED_OMR(data).rgb);
#endif //USE_OCCLUSION_METALLIC_ROUGHNESS_MAP
}
#endif //USE_CUSTOM_ROUGHNESS

#endif //!PRE_PASS

#if !defined(PRE_PASS) || defined(HAS_TRANSPARENCY)
vec4 getTextureColour(in vec4 albedo, in vec2 uv, in uint texOperation) {
#if defined(SKIP_TEX0)
    vec4 colour = albedo;
#else //SKIP_TEX0
    vec4 colour = texture(texDiffuse0, uv);
#endif //SKIP_TEX0

#if !defined(SKIP_TEX1)
#   define TEX_NONE 0
#   define TEX_MODULATE 1
#   define TEX_ADD  2
#   define TEX_SUBTRACT  3
#   define TEX_DIVIDE  4
#   define TEX_SMOOTH_ADD  5
#   define TEX_SIGNED_ADD  6
#   define TEX_DECAL  7
#   define TEX_REPLACE  8

    // Read from the second texture (if any)
    switch (texOperation) {
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
        } break;
    }
#endif //!SKIP_TEX1

    return saturate(colour);
}

#if defined(HAS_TRANSPARENCY)
vec4 getAlbedo(in NodeMaterialData data, in vec2 uv) {
    vec4 albedo = getTextureColour(BaseColour(data), uv, dvd_texOperation(data));
#   if defined(USE_OPACITY_MAP)
#       if defined(USE_OPACITY_MAP_RED_CHANNEL)
            albedo.a = texture(texOpacityMap, uv).r;
#       else //USE_OPACITY_MAP_RED_CHANNEL
            albedo.a = texture(texOpacityMap, uv).a;
#       endif //USE_OPACITY_MAP_RED_CHANNEL
#   endif //USE_OPACITY_MAP
    return albedo;
}
#else //HAS_TRANSPARENCY
#define getAlbedo(DATA, UV) getTextureColour(BaseColour(DATA), UV, dvd_texOperation(DATA))
#endif //HAS_TRANSPARENCY

#endif //!PRE_PASS || HAS_TRANSPARENCY

#if defined(SAMPLER_NORMALMAP_IS_ARRAY)
#define UV_TYPE vec3
#else //SAMPLER_NORMALMAP_IS_ARRAY
#define UV_TYPE vec2
#endif //SAMPLER_NORMALMAP_IS_ARRAY

vec3 getNormalWV(in UV_TYPE uv) {
    vec3 normalWV = VAR._normalWV;
#if defined(COMPUTE_TBN) && !defined(USE_CUSTOM_NORMAL_MAP)
    if (dvd_bumpMethod(MATERIAL_IDX) != BUMP_NONE) {
        normalWV = getTBNWV() * normalize(2.f * texture(texNormalMap, uv).rgb - 1.f);
    }
#endif //COMPUTE_TBN && !USE_CUSTOM_NORMAL_MAP

    normalWV = normalize(normalWV);
#if defined (USE_DOUBLE_SIDED) && !defined(SKIP_DOUBLE_SIDED_NORMALS)
    return gl_FrontFacing ? normalWV : -normalWV;
#else //USE_DOUBLE_SIDED && !SKIP_DOUBLE_SIDED_NORMALS
    return normalWV;
#endif //USE_DOUBLE_SIDED && !SKIP_DOUBLE_SIDED_NORMALS
}

#endif //_MATERIAL_DATA_FRAG_
