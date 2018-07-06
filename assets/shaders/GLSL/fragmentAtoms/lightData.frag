#ifndef _LIGHT_DATA_FRAG_
#define _LIGHT_DATA_FRAG_

#define M_PIDIV180 0.01745329251994329576923690768488

float getLightAttenuationDirectional(const in vec3 lightDirection) {
    return 1.0;
}

float getLightAttenuationPoint(const in vec3 lightDirection) {
    float dis = length(lightDirection);
    float r = dvd_private_light._positionWV.w;
    float att = clamp(1.0 - (dis*dis) / (r*r), 0.0, 1.0);
    att *= att;
    return att;
}

float getLightAttenuationSpot(const in vec3 lightDirection) {
    float att = getLightAttenuationPoint(lightDirection);

    vec4 dirIn = dvd_private_light._directionWV;
    float cosOutterConeAngle = dvd_private_light._colour.w;
    float cosInnerMinusOuterAngle = dirIn.w - cosOutterConeAngle;

    return att * clamp((dot(-normalize(lightDirection), normalize(dirIn.xyz)) - cosOutterConeAngle) / cosInnerMinusOuterAngle, 0.0, 1.0);
    
}

// Light direction is NOT normalized
float getLightAttenuation(const in vec3 lightDirection) {
    int lightType = int(dvd_private_light._options.x);

    return mix(mix(getLightAttenuationSpot(lightDirection),
                   getLightAttenuationPoint(lightDirection),
                   lightType == LIGHT_OMNIDIRECTIONAL),
               getLightAttenuationDirectional(lightDirection),
               lightType == LIGHT_DIRECTIONAL);
}

//non-normalized light direction!
vec3 getLightDirection() {
    int lightType = int(dvd_private_light._options.x);

    return mix(dvd_private_light._positionWV.xyz - VAR._vertexWV.xyz,
               -dvd_private_light._positionWV.xyz,
               vec3(lightType == LIGHT_DIRECTIONAL));

}

#endif