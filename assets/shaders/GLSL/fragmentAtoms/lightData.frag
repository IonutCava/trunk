#ifndef _LIGHT_DATA_FRAG_
#define _LIGHT_DATA_FRAG_

struct LightPropertiesFrag {
    vec3  _lightDir;
    float _NDotL;
    vec3  _viewDirNorm;
    float _att;
    uint  _lightIndex;
};

float computeAttenuationOmni(const in uint lightIndex, const in vec3 lightDirection) {
    float distance = length(lightDirection);
    float radius = dvd_LightSource[lightIndex]._position.w;
    float att = clamp(1.0 - distance*distance / (radius*radius), 0.0, 1.0);
    att *= att;
    if (att > radius) {
        return 0;
    }

    return att;
}

#define M_PIDIV180 0.01745329251994329576923690768488
float computeAttenuationSpot(const in uint lightIndex, const in vec3 lightDirection){
    float att = computeAttenuationOmni(lightIndex, lightDirection);

    vec4 dirIn = dvd_LightSource[lightIndex]._direction;
    float clampedCosine = max(0.0, dot(-lightDirection, normalize(dirIn.xyz)));
    return mix(0.0, att * pow(clampedCosine, dirIn.w), clampedCosine < cos(dvd_LightSource[lightIndex]._position.w * M_PIDIV180));
}

vec3 getLightDirection(const in uint lightIndex, const in vec3 viewDir, const in vec3 normal) {
#if defined(COMPUTE_TBN)
    vec3 lightDir = normalize(viewDir + dvd_LightSource[lightIndex]._position.xyz);
    return normalize(vec3(dot(lightDir, _tangentWV), dot(lightDir, _bitangentWV), dot(lightDir, normal)));
#else
    return normalize(viewDir + dvd_LightSource[lightIndex]._position.xyz);
#endif
}

vec3 getLightDirectionDirectional(const in uint lightIndex, const in vec3 normal) {
#if defined(COMPUTE_TBN)
    vec3 lightDir = -normalize(dvd_LightSource[lightIndex]._position.xyz);
    return normalize(vec3(dot(lightDir, _tangentWV), dot(lightDir, _bitangentWV), dot(lightDir, normal)));
#else
    return -normalize(dvd_LightSource[lightIndex]._position.xyz);
#endif
}

LightPropertiesFrag getLightProperties(const in uint lightIndex, const in vec3 normal) {
    LightPropertiesFrag currentLight;
    currentLight._lightIndex = lightIndex;
    currentLight._viewDirNorm = normalize(_viewDirection);
    switch (dvd_LightSource[lightIndex]._options.x){
        case LIGHT_DIRECTIONAL: {
            currentLight._lightDir = getLightDirectionDirectional(lightIndex, normal);
            currentLight._att = 1.0;
        } break;
        case LIGHT_OMNIDIRECTIONAL: {
            vec3 lightDir = getLightDirection(lightIndex, currentLight._viewDirNorm, normal);
            currentLight._lightDir = lightDir;
            currentLight._att = computeAttenuationOmni(lightIndex, lightDir);
        } break;
        case LIGHT_SPOT: {
            vec3 lightDir = getLightDirection(lightIndex, currentLight._viewDirNorm, normal);
            currentLight._lightDir = lightDir;
            currentLight._att = computeAttenuationSpot(lightIndex, lightDir);
        } break;
    }

    
    currentLight._NDotL = max(dot(currentLight._lightDir, normal), 0.0);

    return currentLight;
}

#endif