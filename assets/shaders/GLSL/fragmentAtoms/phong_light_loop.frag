
struct MaterialProperties {
    vec4 ambient;
    vec4 diffuse;
    vec4 specular;
    vec4 specularValue;
};

#include "shadowMapping.frag"

void phong_omni(in int lightIndex, in float NdotL, in float iSpecular, inout MaterialProperties materialProp, float att) {
    //add the lighting contributions
    materialProp.ambient += dvd_LightSource[lightIndex]._diffuse.w * material[0] * att;
    if (NdotL > 0.0){
        materialProp.diffuse  += vec4(dvd_LightSource[lightIndex]._diffuse.rgb, 1.0)  * material[1] * NdotL * att;
        materialProp.specular += vec4(dvd_LightSource[lightIndex]._specular.rgb, 1.0) * materialProp.specularValue * iSpecular * att;
    }
}

void phong_spotLight(in int lightIndex, in float NdotL, in float iSpecular, inout MaterialProperties materialProp) {
    //Diffuse intensity
    if (NdotL > 0.0){
        float att = _lightInfo._attenuation[lightIndex];
        //add the lighting contributions
        materialProp.ambient += dvd_LightSource[lightIndex]._diffuse.w  * material[0] * att;
        materialProp.diffuse += vec4(dvd_LightSource[lightIndex]._diffuse.rgb, 1.0)  * material[1] * NdotL * att;
        materialProp.specular += vec4(dvd_LightSource[lightIndex]._specular.rgb, 1.0) * materialProp.specularValue  * iSpecular * att;
    }
}

void applyLight(const in int index, const in float NdotL, in float iSpecular, inout MaterialProperties materialProp){
    switch (dvd_lightType[index]){
        case LIGHT_DIRECTIONAL      : phong_omni(index, NdotL, iSpecular, materialProp, 1.0); return;
        case LIGHT_OMNIDIRECTIONAL  : phong_omni(index, NdotL, iSpecular, materialProp, _lightInfo._attenuation[index]); return;
        case LIGHT_SPOT             : phong_spotLight(index, NdotL, iSpecular, materialProp);        return;
    }
}

void applyShadow(const in int index, const in Shadow currentShadowSource, inout float shadowFactor) {
    if (!dvd_lightCastsShadows[index]) return;

    switch (dvd_lightType[index]){
        case LIGHT_DIRECTIONAL     : shadowFactor *= applyShadowDirectional(index, currentShadowSource); return;
        case LIGHT_OMNIDIRECTIONAL : shadowFactor *= applyShadowPoint(index, currentShadowSource);       return;
        case LIGHT_SPOT            : shadowFactor *= applyShadowSpot(index, currentShadowSource);        return;
    }
}

void phong_loop(in int lightCount, in vec3 normal, inout MaterialProperties materialProp){
    vec3 L; 
    vec3 R;
    vec3 viewDirection = normalize(_viewDirection);

    float iSpecular = 0.0;
    // Apply all lighting contributions
    for (int i = 0; i < MAX_LIGHTS_PER_NODE; i++){
        if (lightCount == i) break;

        L = normalize(_lightInfo._lightDirection[i]);
        R = normalize(-reflect(L, normal));
        iSpecular = clamp(pow(max(dot(R, viewDirection), 0.0), material[3].x), 0.0, 1.0);
        applyLight(dvd_lightIndex[i], max(dot(normal, L), 0.0), iSpecular, materialProp);
    }
 }

float shadow_loop(in int lightCount){
    // Early-out if shadows are disabled
    if (!dvd_enableShadowMapping) return 1.0;
       
    float shadow = 1.0;
    for (int i = 0; i < MAX_SHADOW_CASTING_LIGHTS; i++){
        if (lightCount == i) break;

        int lightIndex = dvd_lightIndex[i];
        applyShadow(lightIndex, dvd_ShadowSource[lightIndex], shadow);
    }

    return clamp(shadow, 0.0, 1.0);
}