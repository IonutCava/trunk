#ifndef _LIGHT_DATA_FRAG_
#define _LIGHT_DATA_FRAG_

#define M_PIDIV180 0.01745329251994329576923690768488
// Light direction is NOT normalized
float getLightAttenuation(const in int lightIndex,
                          const in vec3 lightDirection){
    int lightType = dvd_LightSource[lightIndex]._options.x;
    if (lightType == LIGHT_DIRECTIONAL) {
        return 1.0;
    }

    float dis = length(lightDirection);
    float r = dvd_LightSource[lightIndex]._positionWV.w;
    float att = clamp(1.0 - (dis*dis) / (r*r), 0.0, 1.0);
    att *= att;

    if (lightType == LIGHT_SPOT) {
        vec4 dirIn = dvd_LightSource[lightIndex]._directionWV;
        float cosOutterConeAngle = dvd_LightSource[lightIndex]._color.w;
        float cosInnerMinusOuterAngle = dirIn.w - cosOutterConeAngle;
        att *= clamp((dot(-normalize(lightDirection), normalize(dirIn.xyz)) - cosOutterConeAngle) /
                     cosInnerMinusOuterAngle, 0.0, 1.0);
    }
    return att;
}

//non-normalized light direction!
vec3 getLightDirection(const in int lightIndex) {
    if (dvd_LightSource[lightIndex]._options.x == LIGHT_DIRECTIONAL) {
        return -dvd_LightSource[lightIndex]._positionWV.xyz;
    }
     
    return dvd_LightSource[lightIndex]._positionWV.xyz - VAR._vertexWV.xyz;

}

#endif