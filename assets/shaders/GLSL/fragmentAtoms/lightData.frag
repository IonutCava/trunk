#ifndef _LIGHT_DATA_FRAG_
#define _LIGHT_DATA_FRAG_

#define M_PIDIV180 0.01745329251994329576923690768488

float getLightAttenuationPoint(in float radius, in vec3 lightDirection) {
    float dis = length(lightDirection);
    float att = clamp(1.0 - (dis*dis) / (radius*radius), 0.0, 1.0);
    return (att * att);
}

float getLightAttenuationSpot(in Light light, in vec3 lightDirection, in float att) {
    vec4 dirIn = light._directionWV;
    float cosOutterConeAngle = light._colour.w;
    float cosInnerMinusOuterAngle = dirIn.w - cosOutterConeAngle;

    return att * clamp((dot(-normalize(lightDirection), normalize(dirIn.xyz)) - cosOutterConeAngle) / cosInnerMinusOuterAngle, 0.0, 1.0);
}

float getLightAttenuationSpot(in Light light, in vec3 lightDirection) {
    float radius = light._positionWV.w;
    float att = getLightAttenuationPoint(radius, lightDirection);
    return getLightAttenuationSpot(light, lightDirection, att);
}


#endif