#ifndef _MATERIAL_DATA_FRAG_
#define _MATERIAL_DATA_FRAG_

#if defined(OIT_PASS)
#define NEED_DEPTH_TEXTURE
#endif
#include "utility.frag"

//Ref: https://github.com/urho3d/Urho3D/blob/master/bin/CoreData/Shaders/GLSL/PBRLitSolid.glsl
#if defined(USE_ALBEDO_ALPHA) || defined(USE_OPACITY_MAP)
#   define HAS_TRANSPARENCY
#endif

#if !defined(PRE_PASS) || defined(HAS_TRANSPARENCY)
#if !defined(SKIP_TEXTURES)
layout(binding = TEXTURE_UNIT0) uniform sampler2D texDiffuse0;
#   if TEX_OPERATION != TEX_NONE
layout(binding = TEXTURE_UNIT1) uniform sampler2D texDiffuse1;
#   endif //TEX_OPERATION != TEX_NONE
#endif //SKIP_TEXTURES
#endif

#if !defined(PRE_PASS)
layout(binding = TEXTURE_REFLECTION_PLANAR) uniform sampler2D texReflectPlanar;
layout(binding = TEXTURE_REFRACTION_PLANAR) uniform sampler2D texRefractPlanar;
layout(binding = TEXTURE_REFLECTION_CUBE) uniform samplerCubeArray texEnvironmentCube;
#if defined(USE_SPECULAR_MAP)
layout(binding = TEXTURE_SPECULAR) uniform sampler2D texSpecularMap;
#endif //USE_SPECULAR_MAP
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


vec2 getTexCoord() {
#   if defined(USE_PARALLAX_MAPPING)
    return ParallaxNormal(VAR._texCoord, normalize(-VAR._vertexWV.xyz));
#   endif //USE_PARALLAX_MAPPING
    return VAR._texCoord;
}

#if defined(USE_PARALLAX_MAPPING)
uniform float height_scale = 0.1f;

layout(binding = TEXTURE_HEIGHT) uniform sampler2D parallaxDepthMap;

//ref: https://learnopengl.com/Advanced-Lighting/Parallax-Mapping
// Returned parallaxed texCoords
vec2 ParallaxMapping(vec2 texCoords, vec3 viewDir)
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
    vec2 P = viewDir.xy * height_scale;
    vec2 deltaTexCoords = P / numLayers;

    // get initial values
    vec2  currentTexCoords = texCoords;
    float currentDepthMapValue = texture(parallaxDepthMap, currentTexCoords).r;

    while (currentLayerDepth < currentDepthMapValue)
    {
        // shift texture coordinates along direction of P
        currentTexCoords -= deltaTexCoords;
        // get depthmap value at current texture coordinates
        currentDepthMapValue = texture(depthMap, currentTexCoords).r;
        // get depth of next layer
        currentLayerDepth += layerDepth;
    }

    vec2 prevTexCoords = currentTexCoords + deltaTexCoords;

    // get depth after and before collision for linear interpolation
    float afterDepth = currentDepthMapValue - currentLayerDepth;
    float beforeDepth = texture(depthMap, prevTexCoords).r - currentLayerDepth + layerDepth;

    // interpolation of texture coordinates
    float weight = afterDepth / (afterDepth - beforeDepth);
    vec2 finalTexCoords = prevTexCoords * weight + currentTexCoords * (1.0f - weight);

    return finalTexCoords;
}
#endif

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
        float gloss = texture2D(texSpecularMap, uv).r;
        gloss /= mix(power/baked_power, 1.0, gloss);
        return gloss;
    #else
        return 1.0;
    #endif
}
float getShininess(mat4 colourMatrix) {
    return colourMatrix[2].w;
}

float getRoughness(mat4 colourMatrix) {
    return 1.0 - saturate(getShininess(colourMatrix) / 255.0);
}

float getReflectivity(mat4 colourMatrix) {
#if defined(USE_SHADING_PHONG) || defined (USE_SHADING_BLINN_PHONG)
    return getShininess(colourMatrix);
#elif defined(USE_SHADING_TOON)
    // ToDo - will cause compile error
#else //if defined(USE_SHADING_COOK_TORRANCE) || defined(USE_SHADING_OREN_NAYAR)
    float roughness = getRoughness(colourMatrix);
    return roughness * roughness;
#endif
}

vec3 getEmissive(mat4 colourMatrix) {
    return colourMatrix[2].rgb;
}

void setEmissive(mat4 colourMatrix, vec3 value) {
    colourMatrix[2].rgb = value;
}

vec3 getSpecular(mat4 colourMatrix, in vec2 uv) {
#if defined(USE_SPECULAR_MAP)
    return texture(texSpecularMap, uv).rgb;
#else
    return colourMatrix[1].rgb;
#endif
}
#endif //PRE_PASS

float getOpacity(in mat4 colourMatrix, in float albedoAlpha, in vec2 uv) {
#if defined(HAS_TRANSPARENCY)
#   if defined(USE_OPACITY_MAP)
    return texture(texOpacityMap, uv).r;
#   endif
    return albedoAlpha;
#endif

    return 1.0;
}

#if !defined(PRE_PASS) || defined(HAS_TRANSPARENCY)
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

#if TEX_OPERATION != TEX_NONE
    vec4 colour2 = texture(texDiffuse1, uv);

    // Read from the texture
    switch (TEX_OPERATION) {
        default: colour = vec4(0.7743, 0.3188, 0.5465, 1.0);   break; // <hot pink to easily spot it in a crowd
        case TEX_MODULATE: colour *= colour2;       break;
        case TEX_REPLACE: colour = colour2;       break;
        case TEX_SIGNED_ADD: colour += colour2 - 0.5; break;
        case TEX_DIVIDE: colour /= colour2;       break;
        case TEX_SUBTRACT: colour -= colour2;       break;
        case TEX_DECAL: colour = vec4(mix(colour.rgb, colour2.rgb, colour2.a), colour.a); break;
        case TEX_ADD: {
            colour.rgb += colour2.rgb;
            colour.a *= colour2.a;
        }break;
        case TEX_SMOOTH_ADD: {
            colour = (colour + colour2) - (colour * colour2);
        }break;
    }
#endif

    return saturate(colour);
}
#endif //SKIP_TEXTURES

vec4 getAlbedo(in mat4 colourMatrix, in vec2 uv) {

#if defined(SKIP_TEXTURES)
    vec4 albedo = colourMatrix[0];
#else
    vec4 albedo = getTextureColour(uv);
#endif

    albedo.a = getOpacity(colourMatrix, albedo.a, uv);

    return albedo;
}
#endif //!defined(PRE_PASS) || defined(HAS_TRANSPARENCY)

// Computed normals are NOT normalized. Retrieved normals ARE.
vec3 getNormal(in vec2 uv) {
#if defined(PRE_PASS) || !defined(USE_DEFERRED_NORMALS)
    vec3 normal = VAR._normalWV;

#if defined(COMPUTE_TBN) && !defined(USE_CUSTOM_NORMAL_MAP)
    //if (dvd_lodLevel == 0)
    {
        normal = VAR._tbn * getBump(uv);
    }
#endif //COMPUTE_TBN

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