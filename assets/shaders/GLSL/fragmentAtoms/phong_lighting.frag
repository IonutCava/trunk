uniform vec2  dvd_zPlanes;
uniform mat4  material;
uniform bool  dvd_isSelected;
uniform int   lodLevel = 0;

#include "lightInput.cmn"
#include "lightingDefaults.frag"

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

    materialProp.diffuse = vec4(0.0);
    materialProp.ambient = vec4(0.0);
    materialProp.specular = vec4(0.0);
    
#if defined(USE_SPECULAR_MAP)
    materialProp.specularValue = texture(texSpecularMap, texCoord);
#else
    materialProp.specularValue = material[2];
#endif

    phong_loop(normalize(normal));

    float alpha = 1.0;

#if defined(HAS_TRANSPARENCY)
#if defined(USE_OPACITY_DIFFUSE) || defined(USE_OPACITY_DIFFUSE_MAP)
    alpha *= materialProp.diffuse.a * textureColor.a;
#endif
#if defined(USE_OPACITY)
    alpha *= opacity
#endif
#if defined(USE_OPACITY_MAP)
    vec4 opacityMap = texture(texOpacityMap, texCoord);
    alpha *= max(min(opacityMap.r, opacityMap.g), min(opacityMap.b, opacityMap.a));
#endif
    if (useAlphaTest && alpha < ALPHA_DISCARD_THRESHOLD) discard;
#endif

    //Add global ambient value and selection ambient value
    materialProp.ambient += dvd_lightAmbient * material[0] + materialProp.diffuse + (dvd_isSelected ? vec4(1.0) : vec4(0.0));
    //Add material color terms to the final color
    vec4 linearColor = vec4((materialProp.ambient.rgb * textureColor.rgb) + materialProp.specular.rgb, alpha);
    // Gama correction
    // vec3 gamma = vec3(1.0/2.2);
    // linearColor.rgb = pow(linearColor.rgb, gamma);

    //Apply shadowing
    linearColor.rgb *= shadow_loop();

#if defined(_DEBUG)
    _shadowTempInt = dvd_showShadowSplits ? _shadowTempInt : -2;

    switch (_shadowTempInt){
        case -2: return linearColor;
        case -1: return vec4(1.0);
        case  0: return linearColor + vec4(0.15, 0.0,  0.0,  0.0);
        case  1: return linearColor + vec4(0.00, 0.25, 0.0,  0.0);
        case  2: return linearColor + vec4(0.00, 0.0,  0.40, 0.0);
        case  3: return linearColor + vec4(0.15, 0.25, 0.40, 0.0);
    };
#else
    return linearColor;
#endif
}

vec4 Phong(const in vec2 texCoord, const in vec3 normal){
#if defined(SKIP_TEXTURES)
    return Phong(texCoord, normal, vec4(1.0));
#else
    return Phong(texCoord, normal, getTextureColor(texCoord));
#endif
}
