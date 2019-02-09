#ifndef _MATERIAL_DATA_FRAG_
#define _MATERIAL_DATA_FRAG_

#include "utility.frag"

//Ref: https://github.com/urho3d/Urho3D/blob/master/bin/CoreData/Shaders/GLSL/PBRLitSolid.glsl
#if defined(USE_OPACITY_DIFFUSE) || defined(USE_OPACITY_MAP) || defined(USE_OPACITY_DIFFUSE_MAP)
#   define HAS_TRANSPARENCY
#endif

#if !defined(SKIP_TEXTURES)
layout(binding = TEXTURE_UNIT0) uniform sampler2D texDiffuse0;
layout(binding = TEXTURE_UNIT1) uniform sampler2D texDiffuse1;
#endif

layout(binding = TEXTURE_REFLECTION_PLANAR) uniform sampler2D texReflectPlanar;
layout(binding = TEXTURE_REFRACTION_PLANAR) uniform sampler2D texRefractPlanar;

layout(binding = TEXTURE_REFLECTION_CUBE) uniform samplerCubeArray texEnvironmentCube;
layout(binding = TEXTURE_REFRACTION_CUBE) uniform samplerCubeArray texEnvRefractCube;

//Specular and opacity maps are available even for non-textured geometry
#if defined(USE_OPACITY_MAP)
layout(binding = TEXTURE_OPACITY) uniform sampler2D texOpacityMap;
#endif

#if defined(USE_SPECULAR_MAP)
layout(binding = TEXTURE_SPECULAR) uniform sampler2D texSpecularMap;
#endif

// Debug toggles
#if defined(_DEBUG)
uniform bool dvd_LightingOnly = false;
uniform bool dvd_NormalsOnly = false;
#endif

vec3 getProcessedNormal(vec3 normalIn) {
#   if defined (USE_DOUBLE_SIDED)
    return gl_FrontFacing ? normalIn : -normalIn;
#   else
    return normalIn;
#   endif
}

float Gloss(in vec3 bump, in vec2 texCoord)
{
    #if defined(USE_TOKSVIG)
        // Compute the "Toksvig Factor"
        float rlen = 1.0/saturate(length(bump));
        return 1.0/(1.0 + power*(rlen - 1.0));
    #elif defined(USE_TOKSVIG_MAP)
        float baked_power = 100.0;
        // Fetch pre-computed "Toksvig Factor"
        // and adjust for specified power
        float gloss = texture2D(texSpecularMap, texCoord).r;
        gloss /= mix(power/baked_power, 1.0, gloss);
        return gloss;
    #else
        return 1.0;
    #endif
}

#if !defined(SKIP_TEXTURES)
vec4 getTextureColour(in vec2 uv) {
    #define TEX_NONE 0
    #define TEX_MODULATE 1
    #define TEX_ADD  2
    #define TEX_SUBTRACT  3
    #define TEX_DIVIDE  4
    #define TEX_SMOOTH_ADD  5
    #define TEX_SIGNED_ADD  6
    #define TEX_DECAL  7
    #define TEX_REPLACE  8

    vec4 colour = texture(texDiffuse0, uv);

    if (dvd_texOperation == TEX_NONE) {
        return colour;
    }

    vec4 colour2 = texture(texDiffuse1, uv);

    // Read from the texture
    switch (dvd_texOperation) {
        default             : colour = vec4(0.7743, 0.3188, 0.5465, 1.0);   break; // <hot pink to easily spot it in a crowd
        case TEX_MODULATE   : colour *= colour2;       break;
        case TEX_REPLACE    : colour  = colour2;       break;
        case TEX_SIGNED_ADD : colour += colour2 - 0.5; break;
        case TEX_DIVIDE     : colour /= colour2;       break;
        case TEX_SUBTRACT   : colour -= colour2;       break;
        case TEX_DECAL      : colour = vec4(mix(colour.rgb, colour2.rgb, colour2.a), colour.a); break;
        case TEX_ADD        : {
            colour.rgb += colour2.rgb;
            colour.a   *= colour2.a;
        }break;
        case TEX_SMOOTH_ADD : {
            colour = (colour + colour2) - (colour * colour2);
        }break;
    }

    return saturate(colour);
}
#endif

float private_getOpacity();
vec4 private_getAlbedo();
vec3 private_getEmissive();
vec3 private_getSpecular();
float private_getShininess();

#if !defined(CUSTOM_MATERIAL_OPACITY)
float getOpacity() {
    return private_getOpacity();
}
#else
float getOpacity();
#endif

#if !defined(CUSTOM_MATERIAL_ALBEDO)
vec4 getAlbedo() {
    return private_getAlbedo();
}
#else
vec4 getAlbedo();
#endif

#if !defined(CUSTOM_MATERIAL_EMISSIVE)
vec3 getEmissive() {
    return private_getEmissive();
}
#else
vec3 getEmissive();
#endif

#if !defined(CUSTOM_MATERIAL_SPECULAR)
vec3 getSpecular() {
    return private_getSpecular();
}
#else
vec3 getSpecular();
#endif

#if !defined(CUSTOM_MATERIAL_SHININESS)
float getShininess() {
    return private_getShininess();
}
#else
float getShininess();
#endif

float getRoughness() {
    return 1.0 - saturate(getShininess() / 255.0);
}

float getReflectivity() {
#if defined(USE_SHADING_PHONG) || defined (USE_SHADING_BLINN_PHONG)
    return getShininess();
#elif defined(USE_SHADING_TOON)
    // ToDo - will cause compile error
#else //if defined(USE_SHADING_COOK_TORRANCE) || defined(USE_SHADING_OREN_NAYAR)
    float roughness = getRoughness();
    return roughness * roughness;
#endif
}

vec4 private_getDiffuseColour() {

#   if defined(SKIP_TEXTURES)
        return dvd_Matrices[VAR.dvd_baseInstance]._colourMatrix[0];
#   else
        return getTextureColour(VAR._texCoord);
#   endif

    return vec4(1.0);
}

#if defined(USE_OPACITY_DIFFUSE_MAP)
float private_pixel_opacity = -1.0;
#endif


float private_getOpacity() {
#if defined(HAS_TRANSPARENCY)

#   if defined(USE_OPACITY_DIFFUSE)
    return dvd_Matrices[VAR.dvd_baseInstance]._colourMatrix[0].a;
#   endif

#   if defined(USE_OPACITY_MAP)
    vec4 opacityMap = texture(texOpacityMap, VAR._texCoord);
    return max(min(opacityMap.r, opacityMap.g), min(opacityMap.b, opacityMap.a));
#   endif

#   if defined(USE_OPACITY_DIFFUSE_MAP)
    if (private_pixel_opacity < 0) {
        private_pixel_opacity = private_getDiffuseColour().a;
    }

    return private_pixel_opacity;
#   endif

#   endif

    return 1.0;
}

vec4 private_getAlbedo() {
    vec4 albedo = private_getDiffuseColour();

#   if defined(USE_OPACITY_DIFFUSE_MAP)
        private_pixel_opacity = albedo.a;
#   endif

#   if defined(_DEBUG)
        if (dvd_LightingOnly) {
            albedo.rgb = vec3(0.0);
        }
#   endif

    albedo.a = getOpacity();

#if defined(USE_ALPHA_DISCARD)
    if (albedo.a < 1.0 - Z_TEST_SIGMA) {
        discard;
    }
#endif
    return albedo;
}

vec3 private_getEmissive() {
    return dvd_Matrices[VAR.dvd_baseInstance]._colourMatrix[2].rgb;
}

vec3 private_getSpecular() {
#if defined(USE_SPECULAR_MAP)
    return texture(texSpecularMap, VAR._texCoord).rgb;
#else
    return dvd_Matrices[VAR.dvd_baseInstance]._colourMatrix[1].rgb;
#endif
}

float private_getShininess() {
    return dvd_Matrices[VAR.dvd_baseInstance]._colourMatrix[2].w;
}

#endif //_MATERIAL_DATA_FRAG_