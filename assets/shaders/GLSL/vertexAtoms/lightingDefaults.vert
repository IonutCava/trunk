#define MODE_BUMP		1
#define MODE_PARALLAX	2
#define MODE_RELIEF		3

#include "lightInput.cmn"

subroutine void LightInfoType(in vec3 vertexWV);
subroutine uniform LightInfoType LightInfoRoutine;

const float pidiv180 = 0.0174532777777778; //3.14159 / 180.0; // 36 degrees

subroutine(LightInfoType)
void computeLightInfoLOD0(in vec3 vertexWV){
//void LightInfoRoutine0(in vec3 vertexWV){
#if defined(COMPUTE_TBN)
    vec3 tangentWV = normalize(dvd_NormalMatrix * dvd_Tangent);
    vec3 bitangentWV = cross(_normalWV, tangentWV);
    if (length(dvd_Tangent) > 0){
        _normalWV = bitangentWV;
    }

    _viewDirection = vec3(dot(-vertexWV, tangentWV), dot(-vertexWV, bitangentWV), dot(-vertexWV, _normalWV));
#else
    _viewDirection = -vertexWV;
#endif

    vec3 lightDirection;
    float distance = 1.0;
    float lightType = 0.0;
    Light currentLight;

    int currentLightCount = int(ceil(dvd_lightCount / (lodLevel + 1.0f)));

    for (int i = 0; i < MAX_LIGHTS_PER_NODE; i++){
        if (currentLightCount == i) break;

        currentLight = dvd_LightSource[dvd_lightIndex[i]];

        //Directional light => lightType == 0; Spot or Omni => lightType == 1
        lightType = clamp(currentLight._position.w, 0.0, 1.0);

        //lightPosMV.w will be 0 for Directional Lights and 1 for Spot or Omni, so this avoids an "if/else"
        lightDirection = mix(-currentLight._position.xyz, normalize(vertexWV - currentLight._position.xyz), lightType);

        distance = length(lightDirection);
        //either _attenuation == 1 if light is directional or we compute the actual value for omni and spot
        _lightInfo[i]._attenuation = mix(1.0, max(1.0 / (currentLight._attenuation.x + currentLight._attenuation.y * distance + currentLight._attenuation.z * distance * distance), 0.0), lightType);

        // spotlight
        if (currentLight._position.w > 1.5){
            float clampedCosine = max(0.0, dot(-lightDirection, normalize(currentLight._direction.rgb)));
            if (clampedCosine < cos(currentLight._attenuation.w * pidiv180)){ // outside of spotlight cone
                _lightInfo[i]._attenuation = 0.0;
            }
            else{
                _lightInfo[i]._attenuation = _lightInfo[i]._attenuation * pow(clampedCosine, currentLight._direction.w);
            }
        }
#if defined(COMPUTE_TBN)
        _lightInfo[i]._lightDirection = vec3(dot(lightDirection, tangentWV), dot(lightDirection, bitangentWV), dot(lightDirection, _normalWV));
#else
        _lightInfo[i]._lightDirection = lightDirection;
#endif
    }
}

subroutine(LightInfoType)
void computeLightInfoLOD1(in vec3 vertexWV){
//void LightInfoRoutine1(in vec3 vertexWV){
    _viewDirection = -vertexWV;

    vec3 lightDirection;
    float distance = 1.0;
    float lightType = 0.0;
    Light currentLight;

    int currentLightCount = int(ceil(dvd_lightCount / (lodLevel + 1.0f)));

    for (int i = 0; i < MAX_LIGHTS_PER_NODE; i++){
        if (currentLightCount == i) break;

        currentLight = dvd_LightSource[dvd_lightIndex[i]];

        //Directional light => lightType == 0; Spot or Omni => lightType == 1
        lightType = clamp(currentLight._position.w, 0.0, 1.0);

        //lightPosMV.w will be 0 for Directional Lights and 1 for Spot or Omni, so this avoids an "if/else"
        lightDirection = mix(-currentLight._position.xyz, normalize(vertexWV - currentLight._position.xyz), lightType);

        distance = length(lightDirection);
        //either _attenuation == 1 if light is directional or we compute the actual value for omni and spot
        _lightInfo[i]._attenuation = mix(1.0, max(1.0 / (currentLight._attenuation.x + currentLight._attenuation.y * distance + currentLight._attenuation.z * distance * distance), 0.0), lightType);

        // spotlight
        if (currentLight._position.w > 1.5){
            float clampedCosine = max(0.0, dot(-lightDirection, normalize(currentLight._direction.rgb)));
            if (clampedCosine < cos(currentLight._attenuation.w * pidiv180)){ // outside of spotlight cone
                _lightInfo[i]._attenuation = 0.0;
            }
            else{
                _lightInfo[i]._attenuation = _lightInfo[i]._attenuation * pow(clampedCosine, currentLight._direction.w);
            }
        }
        _lightInfo[i]._lightDirection = lightDirection;
    }
}

void computeLightVectors(){
    _normalWV = normalize(dvd_NormalMatrix * dvd_Normal); //<ModelView Normal 
    vec4 vertexWV = dvd_ViewMatrix * _vertexW;
    /*if (lodLevel == 0){
        LightInfoRoutine0(vertexWV.xyz);
    }else{
        LightInfoRoutine1(vertexWV.xyz);
    }*/
    LightInfoRoutine(vertexWV.xyz);
}