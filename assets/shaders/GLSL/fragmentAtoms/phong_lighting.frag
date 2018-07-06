in vec2 _texCoord;
in vec3 _normalWV;
in vec4 _vertexWV;
in vec4 _vertexW;

uniform mat4  material;
uniform int   isSelected = 0;
uniform int   lodLevel = 0;
#if defined(_DEBUG)
uniform bool dvd_showShadowSplits = false;
#endif
#include "lightInput.cmn"

#if defined(USE_OPACITY_DIFFUSE) || defined(USE_OPACITY) || defined(USE_OPACITY_MAP) || defined(USE_OPACITY_DIFFUSE_MAP)
    #define HAS_TRANSPARENCY
#endif

#if defined(HAS_TRANSPARENCY)
uniform float opacity = 1.0;
uniform bool  useAlphaTest = false;
#endif

#ifndef SKIP_TEXTURES
#include "texturing.frag"
#endif

#include "phong_light_loop.frag"

vec4 Phong(in vec2 texCoord, in vec3 normal){

#ifndef SKIP_TEXTURES
    //this shader is generated only for nodes with at least 1 texture
    vec4 tBase;

    //Get the texture color. use Replace for the first texture
    applyTexture(texDiffuse0, textureOperation0, 0, texCoord, tBase);
    
    //If we have a second diffuse texture
    if (textureCount > 1){
        //Apply the second texture over the first
        applyTexture(texDiffuse1, textureOperation1, 0, texCoord, tBase);
    }
#endif

    MaterialProperties materialProp;
    materialProp.diffuse  = vec4(0.0);
    materialProp.ambient  = vec4(0.0);
    materialProp.specular = vec4(0.0);
    materialProp.shadowFactor = 1.0;

    phong_loop(texCoord, normalize(normal), materialProp);
    //Add global ambient value and selection ambient value
    materialProp.ambient += dvd_lightAmbient * material[0];
    materialProp.ambient = mix(materialProp.ambient, vec4(1.0), isSelected);
    //Add material color terms to the final color
    vec4 linearColor = materialProp.ambient + materialProp.diffuse;
   
#ifndef SKIP_TEXTURES
    linearColor *= tBase;
#endif

    linearColor.rgb += materialProp.specular.rgb;

#if defined(HAS_TRANSPARENCY)
#   if defined(USE_OPACITY_DIFFUSE) || defined(USE_OPACITY_DIFFUSE_MAP)
        // nothing. already added
#   elif defined(USE_OPACITY)
        linearColor.a *= opacity
#   elif defined(USE_OPACITY_MAP)
        vec4 opacityMap = texture(texOpacityMap, texCoord);
        linearColor.a = max(min(opacityMap.r, opacityMap.g), min(opacityMap.b, opacityMap.a));
#   endif
    if(useAlphaTest && linearColor.a < ALPHA_DISCARD_THRESHOLD) discard;
#endif

// Gama correction
//  vec3 gamma = vec3(1.0/2.2);
//	linearColor.rgb = pow(linearColor.rgb, gamma);
#if defined(_DEBUG)
    if (dvd_showShadowSplits){
        if (dvd_shadowMappingTempInt == -1)
            linearColor = vec4(1.0);
        if (dvd_shadowMappingTempInt == 0)
            linearColor.r += 0.15;
        if (dvd_shadowMappingTempInt == 1)
            linearColor.g += 0.25;
        if(dvd_shadowMappingTempInt == 2)
            linearColor.b += 0.40;
        if (dvd_shadowMappingTempInt == 3)
            linearColor.rgb += vec3(0.15, 0.25, 0.40);
    }
#endif

    //Apply shadowing
    linearColor.rgb *= (0.4 + 0.6 * materialProp.shadowFactor);

    return linearColor;

}
