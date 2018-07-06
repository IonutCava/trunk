#ifndef _LIGHT_DATA_FRAG_
#define _LIGHT_DATA_FRAG_

#define M_PIDIV180 0.01745329251994329576923690768488
float getLightAttenuation(const in uint lightIndex, const in vec3 lightDirection){
    if (dvd_LightSource[lightIndex]._options.x == LIGHT_DIRECTIONAL) {
        return 1.0;
    }

    float distance = length(lightDirection);
    float radius = dvd_LightSource[lightIndex]._position.w;
    float att = clamp(1.0 - distance*distance / (radius*radius), 0.0, 1.0);
    att *= att;

    if (dvd_LightSource[lightIndex]._options.x == LIGHT_SPOT) {
        vec4 dirIn = dvd_LightSource[lightIndex]._direction;
        float clampedCosine = max(0.0, dot(-lightDirection, normalize(dirIn.xyz)));
        att = mix(0.0, att * pow(clampedCosine, dirIn.w), clampedCosine < cos(dvd_LightSource[lightIndex]._position.w * M_PIDIV180));
    }

    return att;
}

vec4 getLightDirectionAndDot(const in uint lightIndex, const in vec3 normal) {
    vec3 direction = vec3(0.0);
    if (dvd_LightSource[lightIndex]._options.x == LIGHT_DIRECTIONAL) {
#       if defined(COMPUTE_TBN)
            vec3 lightDir = -normalize(dvd_LightSource[lightIndex]._position.xyz);
            direction = normalize(vec3(dot(lightDir, VAR._tangentWV), dot(lightDir, VAR._bitangentWV), dot(lightDir, normal)));
#      else
            direction = -normalize(dvd_LightSource[lightIndex]._position.xyz);
#      endif
    } else {
#       if defined(COMPUTE_TBN)
            vec3 lightDir = normalize(dvd_ViewDirNormAndDist.xyz + dvd_LightSource[lightIndex]._position.xyz);
            direction = normalize(vec3(dot(lightDir, VAR._tangentWV), dot(lightDir, VAR._bitangentWV), dot(lightDir, normal)));
#       else
            direction = normalize(dvd_ViewDirNormAndDist.xyz + dvd_LightSource[lightIndex]._position.xyz);
#      endif
    }

    return vec4(direction, max(dot(direction, normal), 0.0));
}

#endif