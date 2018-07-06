
struct MaterialProperties {
    vec4 ambient;
    vec4 diffuse;
    vec4 specular;
    float shadowFactor;
};

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
    int currentLightCount = int(ceil(dvd_lightCount / (lodLevel + 1.0f)));

    vec3 L; 
    vec3 R;
    vec3 viewDirection = normalize(_viewDirection);
#if defined(USE_SPECULAR_MAP)
    vec4 specularValue = texture(texSpecularMap, texCoord);
#else
    vec4 specularValue = material[2];
#endif

    for(int i = 0; i < MAX_LIGHTS_PER_NODE; i++){
        if(currentLightCount == i) return;

        L = normalize(_lightInfo[i]._lightDirection);
        R = normalize(-reflect(L, normal)); 
        iSpecular = clamp(pow(max(dot(R, viewDirection), 0.0), material[3].x ), 0.0, 1.0);
        NdotL = max(dot(normal, L), 0.0);
        applyLight(i, dvd_lightType[dvd_lightIndex[i]], dvd_lightCastsShadows[dvd_lightIndex[i]], specularValue, materialProp);
    }
 
}