#ifndef _LIGHT_DATA_FRAG_
#define _LIGHT_DATA_FRAG_

#define M_PIDIV180 0.01745329251994329576923690768488

float getLightAttenuationPoint(const in Light light, const in vec3 lightDirection) {
    float dis = length(lightDirection);
    float r = light._positionWV.w;
    float att = clamp(1.0 - (dis*dis) / (r*r), 0.0, 1.0);
    att *= att;
    return att;
}

float getLightAttenuationSpot(const in Light light, const in vec3 lightDirection) {

    float att = getLightAttenuationPoint(light, lightDirection);

    vec4 dirIn = light._directionWV;
    float cosOutterConeAngle = light._colour.w;
    float cosInnerMinusOuterAngle = dirIn.w - cosOutterConeAngle;

    return att * clamp((dot(-normalize(lightDirection), normalize(dirIn.xyz)) - cosOutterConeAngle) / cosInnerMinusOuterAngle, 0.0, 1.0);
    
}

#endif