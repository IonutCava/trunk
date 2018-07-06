uniform int lodLevel = 0;

#include "lightInput.cmn"
#include "lightingDefaults.frag"

#if defined(USE_OPACITY_DIFFUSE) || defined(USE_OPACITY_MAP) || defined(USE_OPACITY_DIFFUSE_MAP)
    #define HAS_TRANSPARENCY
#endif

//Specular and opacity maps are available even for non-textured geometry
#if defined(USE_OPACITY_MAP)
//Opacity and specular maps
//layout(binding = TEXTURE_OPACITY) uniform sampler2D texOpacityMap;
#endif
#if defined(USE_SPECULAR_MAP)
//layout(binding = TEXTURE_SPECULAR) uniform sampler2D texSpecularMap;
#endif

#if !defined(SKIP_TEXTURES)
#include "texturing.frag"
#endif

struct MaterialProperties {
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    vec3 specularValue;
};

#include "phong_light_loop.frag"

vec4 Phong(const in vec2 texCoord, const in vec3 normal, in vec4 textureColor){

    float alpha = 1.0;
#if defined(HAS_TRANSPARENCY)
#if defined(USE_OPACITY_DIFFUSE_MAP)
    alpha *= textureColor.a;
#endif
#if defined(USE_OPACITY_DIFFUSE)
    alpha *= dvd_MatDiffuse.a;
#endif
#if defined(USE_OPACITY_MAP)
    //vec4 opacityMap = texture(texOpacityMap, texCoord);
    //alpha *= max(min(opacityMap.r, opacityMap.g), min(opacityMap.b, opacityMap.a));
#endif
    /*if (dvd_useAlphaTest && alpha < ALPHA_DISCARD_THRESHOLD)
        discard;*/
#endif

    MaterialProperties materialProp;
    
#if defined(USE_SPECULAR_MAP)
    //materialProp.specularValue = texture(texSpecularMap, texCoord).rgb;
#else
    materialProp.specularValue = dvd_MatSpecular;
#endif

    phong_loop(normalize(gl_FrontFacing ? normal : -normal), materialProp);
    //Add global ambient value and selection ambient value
    vec3 color = (materialProp.ambient + dvd_lightAmbient * dvd_MatAmbient) + materialProp.diffuse;

    if(dvd_isSelected)
        color *= 2;

    color *= textureColor.rgb;
    color += materialProp.specular;
    // Gama correction
    // color = pow(color, vec3(1.0/2.2));
    // Apply shadowing
    color *= shadow_loop();

#if defined(_DEBUG) && defined(DEBUG_SHADOWMAPPING)
    switch (dvd_showShadowSplits ? _shadowTempInt : -2){
        case -2: return vec4(color, alpha);
        case -1: return vec4(1.0);
        case  0: return vec4(color, alpha) + vec4(0.15, 0.0,  0.0,  0.0);
        case  1: return vec4(color, alpha) + vec4(0.00, 0.25, 0.0,  0.0);
        case  2: return vec4(color, alpha) + vec4(0.00, 0.0,  0.40, 0.0);
        case  3: return vec4(color, alpha) + vec4(0.15, 0.25, 0.40, 0.0);
    };
#else
    return vec4(color, alpha);
#endif
}

vec4 Phong(const in vec2 texCoord, const in vec3 normal){
#if defined(SKIP_TEXTURES)
    return Phong(texCoord, normal, vec4(1.0));
#else
    return Phong(texCoord, normal, getTextureColor(texCoord));
#endif
}