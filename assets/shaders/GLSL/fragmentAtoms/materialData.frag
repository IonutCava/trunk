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

layout(binding = TEXTURE_GBUFFER_EXTRA)  uniform sampler2D texGBufferExtra;

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

#ifndef F0
#define F0 vec3(0.04f)
#endif

#if defined(USE_CUSTOM_POM)
vec3 getTBNViewDir();
#else //USE_CUSTOM_POM
vec3 getTBNViewDir() {
#   if defined(COMPUTE_POM)
    return VAR._tbnViewDir;
#   else //COMPUTE_POM
    return vec3(0.0f);
#   endif//COMPUTE_POM
}
#endif //USE_CUSTOM_POM

#if defined(USE_CUSTOM_TBN)
// Should cause compile errors if we try to use these without defining them
// so it should be fine. Look at terrainTess.glsl for an example on how to avoid these
mat3 getTBNWV();
#else //USE_CUSTOM_TBN
mat3 getTBNWV() {
#if defined(PRE_PASS) && !defined(HAS_PRE_PASS_DATA)
    return mat3(1.0f);
#else //PRE_PASS && !HAS_PRE_PASS_DATA
#   if defined(COMPUTE_TBN)
    return VAR._tbnWV;
#   else //COMPUTE_TBN
    return mat3(dvd_ViewMatrix);
#   endif//COMPUTE_TBN
#endif //PRE_PASS && !HAS_PRE_PASS_DATA
}
#endif//USE_CUSTOM_TBN

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

float getSSAO() {
#if defined(USE_SSAO)
    return texture(texGBufferExtra, dvd_screenPositionNormalised).r;
#else
    return 1.0f;
#endif
}

vec3 getSpecular(in vec3 diffColour, in float metallic) {
    return mix(F0, diffColour, metallic);
}

#if defined(USE_CUSTOM_ROUGHNESS)
vec3 getOcclusionMetallicRoughness(in mat4 colourMatrix, in vec2 uv);
#else //USE_CUSTOM_ROUGHNESS
vec3 getOcclusionMetallicRoughness(in mat4 colourMatrix, in vec2 uv) {
#if defined(USE_OCCLUSION_METALLIC_ROUGHNESS_MAP)
    vec3 omr = saturate(texture(texOcclusionMetallicRoughness, uv).rgb);
#if defined(PBR_SHADING)
    return omr;
#else
    // Convert specular to roughness ... roughly? really wrong, but should work I guesssssssssss. -Ionut
    return saturate(vec3(Occlusion(colourMatrix),
                         Metallic(colourMatrix),
                         1.0f - omr[2]));
#endif
#else
    return saturate(vec3(Occlusion(colourMatrix),
                         Metallic(colourMatrix),
                         Roughness(colourMatrix)));
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
    if (TexOperation != TEX_NONE) {
        return texture(texDiffuse1, uv);
    }
        
    return albedo;
#else
    vec4 colour = texture(texDiffuse0, uv);
    // Read from the second texture (if any)
    switch (TexOperation) {
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
vec3 getNormalWV(in vec2 uv) {
#if defined(PRE_PASS) || !defined(USE_DEFERRED_NORMALS)

#if defined(PRE_PASS) && !defined(HAS_PRE_PASS_DATA)
    return vec3(0.0f);
#else //PRE_PASS && !HAS_PRE_PASS_DATA

    vec3 normalWV = VAR._normalWV;

#   if defined(COMPUTE_TBN) && !defined(USE_CUSTOM_NORMAL_MAP)
        if (BumpMethod != BUMP_NONE) {
            normalWV = getTBNWV() * normalize(2.0f * texture(texNormalMap, uv).rgb - 1.0f);
        }
#   endif //COMPUTE_TBN

#   if defined (USE_DOUBLE_SIDED)
        if (!gl_FrontFacing) {
            normalWV = -normalWV;
        }
#   endif //USE_DOUBLE_SIDED

    return normalWV;
#endif //PRE_PASS && !HAS_PRE_PASS_DATA
#else //PRE_PASS
    return normalize(unpackNormal(texture(texNormalMap, dvd_screenPositionNormalised).rg));
#endif //PRE_PASS
}

#endif //_MATERIAL_DATA_FRAG_