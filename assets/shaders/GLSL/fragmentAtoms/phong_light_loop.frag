#ifndef _PHONG_LIGHT_LOOP_FRAG_
#define _PHONG_LIGHT_LOOP_FRAG_

#include "shadowMapping.frag"

void phong_omni(in uint lightIndex, in float NdotL, in float iSpecular, float att, inout MaterialProperties materialProp) {
    //add the lighting contributions
    materialProp.ambient += dvd_MatAmbient * dvd_LightSource[lightIndex]._diffuse.a * att;
    if (NdotL > 0.0){
        materialProp.diffuse  += dvd_LightSource[lightIndex]._diffuse.rgb * dvd_MatDiffuse.rgb * NdotL * att;
        materialProp.specular += dvd_LightSource[lightIndex]._specular.rgb * materialProp.specularValue * iSpecular * att;
    }
}

void phong_spot(in uint lightIndex, in float NdotL, in float iSpecular, float att, inout MaterialProperties materialProp) {
    //Diffuse intensity
    if (NdotL > 0.0){
        //add the lighting contributions
        materialProp.ambient  += dvd_MatAmbient * dvd_LightSource[lightIndex]._diffuse.w  * att;
        materialProp.diffuse  += dvd_LightSource[lightIndex]._diffuse.rgb  * dvd_MatDiffuse.rgb * NdotL * att;
        materialProp.specular += dvd_LightSource[lightIndex]._specular.rgb * materialProp.specularValue  * iSpecular * att;
    }
}

void getLightProperties(in vec3 lightDir, in vec3 viewDirection, in vec3 normal, inout float NDotL, inout float iSpecular) {
#if defined(COMPUTE_TBN)
    vec3 L = normalize(vec3(dot(lightDir, _tangentWV), dot(lightDir, _bitangentWV), dot(lightDir, _normalWV)));
    vec3 R = normalize(-reflect(L, normal));
    NDotL = max(dot(normal, L), 0.0);
#else
    vec3 R = normalize(-reflect(lightDir, normal));
    NDotL = max(dot(normal, lightDir), 0.0);
#endif
    iSpecular = clamp(pow(max(dot(R, viewDirection), 0.0), dvd_MatShininess), 0.0, 1.0);
}

#define M_PIDIV180 0.01745329251994329576923690768488

float computeAttenuationSpot(const in uint lightIndex, const in vec3 lightDirection){
    float distance = length(lightDirection);

    vec4 attIn = dvd_LightSource[lightIndex]._attenuation;
    vec4 dirIn = dvd_LightSource[lightIndex]._direction;

    float att = max(1.0 / (attIn.x + (attIn.y * distance) + (attIn.z * distance * distance)), 0.0);

    float clampedCosine = max(0.0, dot(-lightDirection, normalize(dirIn.xyz)));
    return mix(0.0, att * pow(clampedCosine, dirIn.w), clampedCosine < cos(dvd_LightSource[lightIndex]._specular.w * M_PIDIV180));
}

float computeAttenuationOmni(const in uint lightIndex, const in vec3 lightDirection){
    float distance = length(lightDirection);

    vec4 attIn = dvd_LightSource[lightIndex]._attenuation;

    return max(1.0 / (attIn.x + (attIn.y * distance) + (attIn.z * distance * distance)), 0.0);
}

void phong_loop(in vec3 normal, inout MaterialProperties materialProp){
    vec3 viewDirection = normalize(_viewDirection);
    vec3 lightDir = vec3(0.0);
    float iSpecular = 0.0;
    float NDotL = 0.0;
    // Apply all lighting contributions
    for (uint i = 0; i < MAX_LIGHTS_PER_SCENE; i++){
        if (_lightCount == i) break;

        switch (uint(dvd_LightSource[i]._position.w)){
            case LIGHT_DIRECTIONAL      : {
                lightDir = -normalize(dvd_LightSource[i]._position.xyz);
                getLightProperties(lightDir, viewDirection, normal, NDotL, iSpecular);
                phong_omni(i, NDotL, iSpecular, 1.0, materialProp);
            } break;
            case LIGHT_OMNIDIRECTIONAL  : {
                lightDir = normalize(viewDirection + dvd_LightSource[i]._position.xyz);
                getLightProperties(lightDir, viewDirection, normal, NDotL, iSpecular);
                phong_omni(i, NDotL, iSpecular, computeAttenuationOmni(i, lightDir), materialProp);
            } break;
            case LIGHT_SPOT             : {
                lightDir = normalize(viewDirection + dvd_LightSource[i]._position.xyz);
                getLightProperties(lightDir, viewDirection, normal, NDotL, iSpecular);
                phong_spot(i, NDotL, iSpecular, computeAttenuationSpot(i, lightDir), materialProp);
            } break;
        }
    }

    materialProp.diffuse  = clamp(materialProp.diffuse,  vec3(0.0), vec3(1.0));
    materialProp.ambient  = clamp(materialProp.ambient,  vec3(0.0), vec3(1.0));
    materialProp.specular = clamp(materialProp.specular, vec3(0.0), vec3(1.0));
 }

float shadow_loop(){
    // Early-out if shadows are disabled
    if (!dvd_shadowMapping) return 1.0;
       
    float shadow = 1.0;
    for (uint i = 0; i < MAX_SHADOW_CASTING_LIGHTS; i++){
        if (_lightCount == i) break;

        if (dvd_LightSource[i]._options.x == 1)
            switch (uint(dvd_LightSource[i]._position.w)){
                case LIGHT_DIRECTIONAL     : shadow *= applyShadowDirectional(i, dvd_ShadowSource[i]); break;
                //case LIGHT_OMNIDIRECTIONAL : shadow *= applyShadowPoint(i, dvd_ShadowSource[i]);       break;
                //case LIGHT_SPOT            : shadow *= applyShadowSpot(i, dvd_ShadowSource[i]);        break;
            }
    }

    return clamp(shadow, 0.2, 1.0);
}

#endif //_PHONG_LIGHT_LOOP_FRAG_