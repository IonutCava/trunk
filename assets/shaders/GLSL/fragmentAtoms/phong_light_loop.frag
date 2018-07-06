
struct MaterialProperties {
    vec4 ambient;
    vec4 diffuse;
    vec4 specular;
    vec4 specularValue;
} materialProp;

#include "shadowMapping.frag"

void phong_omni(in uint lightIndex, in float NdotL, in float iSpecular, float att) {
    //add the lighting contributions
    materialProp.ambient += dvd_LightSourceVisual[lightIndex]._diffuse.w * material[0] * att;
    if (NdotL > 0.0){
        materialProp.diffuse  += vec4(dvd_LightSourceVisual[lightIndex]._diffuse.rgb, 1.0)  * material[1] * NdotL * att;
        materialProp.specular += vec4(dvd_LightSourceVisual[lightIndex]._specular, 1.0) * materialProp.specularValue * iSpecular * att;
    }
}

void phong_spotLight(in uint lightIndex, in float NdotL, in float iSpecular) {
    //Diffuse intensity
    if (NdotL > 0.0){
        float att = _lightInfo._attenuation[lightIndex];
        //add the lighting contributions
        materialProp.ambient += dvd_LightSourceVisual[lightIndex]._diffuse.w  * material[0] * att;
        materialProp.diffuse += vec4(dvd_LightSourceVisual[lightIndex]._diffuse.rgb, 1.0)  * material[1] * NdotL * att;
        materialProp.specular += vec4(dvd_LightSourceVisual[lightIndex]._specular, 1.0) * materialProp.specularValue  * iSpecular * att;
    }
}

void phong_loop(in vec3 normal){
    vec3 L, R; 
    vec3 viewDirection = normalize(_viewDirection);
    uint lightIndex = 0;
    float iSpecular = 0.0;
    // Apply all lighting contributions
    for (uint i = 0; i < MAX_LIGHTS_PER_NODE; i++){
        if (_lightInfo._lightCount == i) break;

        lightIndex = dvd_perNodeLightData[i].x;
        L = normalize(_lightInfo._lightDirection[i]);
        R = normalize(-reflect(L, normal));
        iSpecular = clamp(pow(max(dot(R, viewDirection), 0.0), material[3].x), 0.0, 1.0);

        switch (dvd_perNodeLightData[lightIndex].y){
            case LIGHT_DIRECTIONAL      : phong_omni(lightIndex, max(dot(normal, L), 0.0), iSpecular, 1.0); break;
            case LIGHT_OMNIDIRECTIONAL  : phong_omni(lightIndex, max(dot(normal, L), 0.0), iSpecular, _lightInfo._attenuation[lightIndex]); break;
            case LIGHT_SPOT             : phong_spotLight(lightIndex, max(dot(normal, L), 0.0), iSpecular); break;
        }
    }
 }

float shadow_loop(){
    // Early-out if shadows are disabled
    if (!dvd_enableShadowMapping) return 1.0;
       
    float shadow = 1.0;
    uint lightIndex = 0;
    for (uint i = 0; i < MAX_SHADOW_CASTING_LIGHTS; i++){
        if (_lightInfo._lightCount == i) break;

        lightIndex = dvd_perNodeLightData[i].x;

        if (dvd_perNodeLightData[lightIndex].z == 0) return 1.0;

        switch (dvd_perNodeLightData[lightIndex].y){
            case LIGHT_DIRECTIONAL     : shadow *= applyShadowDirectional(lightIndex, dvd_ShadowSource[lightIndex]); break;
            case LIGHT_OMNIDIRECTIONAL : shadow *= applyShadowPoint(lightIndex, dvd_ShadowSource[lightIndex]);       break;
            case LIGHT_SPOT            : shadow *= applyShadowSpot(lightIndex, dvd_ShadowSource[lightIndex]);        break;
        }
    }

    return clamp(shadow, 0.2, 1.0);
}