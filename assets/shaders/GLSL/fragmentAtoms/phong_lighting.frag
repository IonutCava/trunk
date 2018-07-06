in vec2 _texCoord;
in vec4 _vertexW;

uniform vec2  dvd_zPlanes;
uniform mat4  material;
uniform int   isSelected = 0;
uniform int   lodLevel = 0;

#include "lightInput.cmn"

#if defined(USE_OPACITY_DIFFUSE) || defined(USE_OPACITY) || defined(USE_OPACITY_MAP) || defined(USE_OPACITY_DIFFUSE_MAP)
    #define HAS_TRANSPARENCY
    uniform float opacity = 1.0;
    uniform bool  useAlphaTest = false;
#endif

//Specular and opacity maps are available even for non-textured geometry
#if defined(USE_OPACITY_MAP)
//Opacity and specular maps
layout(binding = TEXTURE_OPACITY) uniform sampler2D texOpacityMap;
#endif
#if defined(USE_SPECULAR_MAP)
layout(binding = TEXTURE_SPECULAR) uniform sampler2D texSpecularMap;
#endif

#if !defined(SKIP_TEXTURES)
#include "texturing.frag"
#endif

#include "phong_light_loop.frag"

vec4 Phong(const in vec2 texCoord, const in vec3 normal, const in vec4 textureColor){

    int currentLightCount = int(ceil(dvd_lightCount / (lodLevel + 1.0f)));

    MaterialProperties materialProp;
    materialProp.diffuse = vec4(0.0);
    materialProp.ambient = vec4(0.0);
    materialProp.specular = vec4(0.0);
    
#if defined(USE_SPECULAR_MAP)
    materialProp.specularValue = texture(texSpecularMap, texCoord);
#else
    materialProp.specularValue = material[2];
#endif

    phong_loop(currentLightCount, normalize(normal), materialProp);
    //Add global ambient value and selection ambient value
    materialProp.ambient += dvd_lightAmbient * material[0];
    materialProp.ambient = mix(materialProp.ambient, vec4(1.0), isSelected);
    //Add material color terms to the final color
    vec4 linearColor = vec4(((materialProp.ambient.rgb + materialProp.diffuse.rgb) * textureColor.rgb) + materialProp.specular.rgb, 1.0);

#if defined(USE_OPACITY_DIFFUSE) || defined(USE_OPACITY_DIFFUSE_MAP)
    linearColor.a *= materialProp.diffuse.a * textureColor.a;
#endif
#if defined(USE_OPACITY)
    linearColor.a *= opacity
#endif
#if defined(USE_OPACITY_MAP)
    vec4 opacityMap = texture(texOpacityMap, texCoord);
    linearColor.a *= max(min(opacityMap.r, opacityMap.g), min(opacityMap.b, opacityMap.a));
#endif
#if defined(HAS_TRANSPARENCY)
    if (useAlphaTest && linearColor.a < ALPHA_DISCARD_THRESHOLD) discard;
#endif

    // Gama correction
    // vec3 gamma = vec3(1.0/2.2);
    // linearColor.rgb = pow(linearColor.rgb, gamma);

    //Apply shadowing
    linearColor.rgb *= (0.2 + 0.8 * shadow_loop(currentLightCount));
#if defined(_DEBUG)
    if (dvd_showShadowSplits){
        if (_shadowTempInt == -1)
            linearColor = vec4(1.0);
        if (_shadowTempInt == 0)
            linearColor.r += 0.15;
        if (_shadowTempInt == 1)
            linearColor.g += 0.25;
        if (_shadowTempInt == 2)
            linearColor.b += 0.40;
        if (_shadowTempInt == 3)
            linearColor.rgb += vec3(0.15, 0.25, 0.40);
    }
#endif
    return linearColor;
}

vec4 Phong(const in vec2 texCoord, const in vec3 normal){
#if defined(SKIP_TEXTURES)
    return Phong(texCoord, normal, vec4(1.0));
#else
    return Phong(texCoord, normal, getTextureColor(texCoord));
#endif
}

