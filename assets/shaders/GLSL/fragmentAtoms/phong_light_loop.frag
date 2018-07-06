in vec3 _lightDirection[MAX_LIGHT_COUNT];
smooth in float _attenuation[MAX_LIGHT_COUNT];
in vec3 _viewDirection;

struct MaterialProperties {
    vec4 ambient;
    vec4 diffuse;
    vec4 specular;
    float shadowFactor;
};

uniform bool dvd_lightCastsShadows[MAX_LIGHT_COUNT];
uniform int  dvd_lightType[MAX_LIGHT_COUNT];
uniform int  dvd_lightCount;
uniform vec4 dvd_lightAmbient;

//Specular and opacity maps are available even for non-textured geometry
#if defined(USE_OPACITY_MAP)
//Opacity and specular maps
uniform sampler2D texOpacityMap;
#endif
#if defined(USE_SPECULAR_MAP)
uniform sampler2D texSpecularMap;
#endif

const int LIGHT_DIRECTIONAL = 0;
const int LIGHT_OMNIDIRECTIONAL = 1;
const int LIGHT_SPOT = 2;

#include "phong_point_light.frag"
#include "phong_spot_light.frag"
#include "phong_directional_light.frag"
#include "shadowMapping.frag"

float iSpecular;
float NdotL;

void applyLight(const in int index, const in int type, const in bool castsShadows, const in vec4 specular, inout MaterialProperties materialProp) {

    if(type == LIGHT_DIRECTIONAL ) {
        phong_directionalLight(index, iSpecular, NdotL, specular, materialProp);

        if(!dvd_enableShadowMapping || index >= MAX_SHADOW_CASTING_LIGHTS || !castsShadows) return;
        applyShadowDirectional(index, materialProp.shadowFactor); 

    } else {
        if(type == LIGHT_OMNIDIRECTIONAL) {
            phong_pointLight(index, iSpecular, NdotL, specular, materialProp);

            if(!dvd_enableShadowMapping || index >= MAX_SHADOW_CASTING_LIGHTS || !castsShadows) return;
            applyShadowPoint(index, materialProp.shadowFactor); 

        } else { //LIGHT_SPOT
            phong_spotLight(index, iSpecular, NdotL, specular, materialProp);

            if(!dvd_enableShadowMapping || index >= MAX_SHADOW_CASTING_LIGHTS || !castsShadows) return;
            applyShadowSpot(index, materialProp.shadowFactor);
        }
    }
}

void phong_loop(in vec2 texCoord, in vec3 normal, inout MaterialProperties materialProp){
    int currentLightCount = int(ceil(dvd_lightCount / (lodLevel + 1)));

    vec3 L; vec3 R;

    vec3 viewDirection = normalize(_viewDirection);
#if defined(USE_SPECULAR_MAP)
    vec4 specularValue = texture(texSpecularMap, texCoord);
#else
    vec4 specularValue = material[2];
#endif
     
    if(currentLightCount == 0)  return;
    L = normalize(_lightDirection[0]);
    R = normalize(-reflect(L,normal));    //Specular intensity based on material shininess
    iSpecular = clamp(pow(max(dot(R, viewDirection), 0.0), material[3].x ), 0.0, 1.0);
    NdotL = max(dot(normal, L), 0.0);
    applyLight(0, dvd_lightType[0], dvd_lightCastsShadows[0], specularValue, materialProp);

#if MAX_LIGHT_COUNT >= 2
    if(currentLightCount == 1)  return;
    L = normalize(_lightDirection[1]);
    R = normalize(-reflect(L,normal)); 
    iSpecular = clamp(pow(max(dot(R, viewDirection), 0.0), material[3].x ), 0.0, 1.0);
    NdotL = max(dot(normal, L), 0.0);
    applyLight(1, dvd_lightType[1], dvd_lightCastsShadows[1], specularValue, materialProp);
#endif
#if MAX_LIGHT_COUNT >= 3
    if(currentLightCount == 2)  return;
    L = normalize(_lightDirection[2]);
    R = normalize(-reflect(L,normal)); 
    iSpecular = clamp(pow(max(dot(R, viewDirection), 0.0), material[3].x ), 0.0, 1.0);
    NdotL = max(dot(normal, L), 0.0);
    applyLight(2, dvd_lightType[2], dvd_lightCastsShadows[2], specularValue, materialProp);
#endif
#if MAX_LIGHT_COUNT >= 4
    if(currentLightCount == 3) return;
    L = normalize(_lightDirection[3]);
    R = normalize(-reflect(L,normal)); 
    iSpecular = clamp(pow(max(dot(R, viewDirection), 0.0), material[3].x ), 0.0, 1.0);
    NdotL = max(dot(normal, L), 0.0);
    applyLight(3, dvd_lightType[3], dvd_lightCastsShadows[3], specularValue, materialProp);
#endif
#if MAX_LIGHT_COUNT >= 5
    if(currentLightCount == 4) return;
    L = normalize(_lightDirection[4]);
    R = normalize(-reflect(L,normal)); 
    iSpecular = clamp(pow(max(dot(R, viewDirection), 0.0), material[3].x ), 0.0, 1.0);
    NdotL = max(dot(normal, L), 0.0);
    applyLight(4, dvd_lightType[4], dvd_lightCastsShadows[4], specularValue, materialProp);
#endif
#if MAX_LIGHT_COUNT >= 6
    if(currentLightCount == 5) return;
    L = normalize(_lightDirection[5]);
    R = normalize(-reflect(L,normal)); 
    iSpecular = clamp(pow(max(dot(R, viewDirection), 0.0), material[3].x ), 0.0, 1.0);
    NdotL = max(dot(normal, L), 0.0);
    applyLight(5, dvd_lightType[5], dvd_lightCastsShadows[5], specularValue, materialProp);
#endif
#if MAX_LIGHT_COUNT >= 7
    if(currentLightCount == 6) return;
    L = normalize(_lightDirection[6]);
    R = normalize(-reflect(L,normal)); 
    iSpecular = clamp(pow(max(dot(R, viewDirection), 0.0), material[3].x ), 0.0, 1.0);
    NdotL = max(dot(normal, L), 0.0);
    applyLight(6, dvd_lightType[6], dvd_lightCastsShadows[6], specularValue, materialProp);
#endif
#if MAX_LIGHT_COUNT == 8
    if(currentLightCount == 7) return;
    L = normalize(_lightDirection[7]);
    R = normalize(-reflect(L,normal)); 
    iSpecular = clamp(pow(max(dot(R, viewDirection), 0.0), material[3].x ), 0.0, 1.0);
    NdotL = max(dot(normal, L), 0.0);
    applyLight(7, dvd_lightType[7], dvd_lightCastsShadows[7], specularValue, materialProp);
#endif
#if MAX_LIGHT_COUNT > 8
    ///Apply the rest of the lights
    for(int i = 8; i =< MAX_LIGHT_COUNT; i++){
        if(currentLightCount == i) return;
        L = normalize(_lightDirection[i]);
        R = normalize(-reflect(L,normal)); 
        iSpecular = clamp(pow(max(dot(R, viewDirection), 0.0), material[3].x ), 0.0, 1.0);
        NdotL = max(dot(normal, L), 0.0);
        applyLight(i, dvd_lightType[i], dvd_lightCastsShadows[i], specularValue, materialProp);
    }
#endif
    
}