in vec2 _texCoord;
in vec3 _normalWV;

uniform mat4  material;
uniform float opacity = 1.0;
uniform int   isSelected = 0;
uniform bool  useAlphaTest = false;

#include "lightInput.cmn"

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
    if(textureCount > 1){
        //Apply the second texture over the first
        applyTexture(texDiffuse1, textureOperation1, 0, texCoord, tBase);
    }

#endif

    MaterialProperties materialProp;
    materialProp.shadowFactor = 1.0;
    phong_loop(texCoord, normalize(normal), materialProp);
    //Add global ambient value
    materialProp.ambient += dvd_lightAmbient * material[0];
    //Add selection ambient value
    materialProp.ambient = mix(materialProp.ambient, vec4(1.0), isSelected);

    //Add material color terms to the final color
    vec4 linearColor = materialProp.ambient + materialProp.diffuse;
   
#ifndef SKIP_TEXTURES
    linearColor *= tBase;
#endif

    linearColor.rgb += materialProp.specular.rgb;
    //Apply shadowing
    linearColor.rgb *= (0.4 + 0.6 * materialProp.shadowFactor);

#if defined(USE_OPACITY_DIFFUSE)
    // nothing. already added
    if(useAlphaTest && linearColor.a < ALPHA_DISCARD_THRESHOLD) discard;
#elif defined(USE_OPACITY)
    linearColor.a *= opacity
    if(useAlphaTest && linearColor.a < ALPHA_DISCARD_THRESHOLD) discard;
#elif defined(USE_OPACITY_MAP)
    linearColor.a = texture(texOpacityMap, texCoord).a;
    if(useAlphaTest && linearColor.a < ALPHA_DISCARD_THRESHOLD) discard;
#elif defined(USE_OPACITY_DIFFUSE_MAP)
    // nothing. already added
    if(useAlphaTest && linearColor.a < ALPHA_DISCARD_THRESHOLD) discard;
#endif

    //vec3 gamma = vec3(1.0/2.2);
//	return vec4(pow(linearColor, gamma), linearColor.a);
    return linearColor;

}
