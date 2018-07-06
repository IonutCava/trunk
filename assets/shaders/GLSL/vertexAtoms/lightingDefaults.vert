#include "lightInput.cmn"

subroutine void LightInfoType(in vec3 vertexWV);
subroutine uniform LightInfoType LightInfoRoutine;

const float pidiv180 = 0.0174532777777778; //3.14159 / 180.0; // 36 degrees

float computeAttenuation(in Light currentLight, in vec3 lightDirection, in int lightType){
    float distance = length(lightDirection);
    float att = max(1.0 / (currentLight._attenuation.x + (currentLight._attenuation.y * distance) + (currentLight._attenuation.z * distance * distance)), 0.0);

    if (lightType == LIGHT_SPOT){
        float clampedCosine = max(0.0, dot(-lightDirection, normalize(currentLight._direction.rgb)));
        return mix(0.0, att * pow(clampedCosine, currentLight._direction.w), clampedCosine < cos(currentLight._attenuation.w * pidiv180));
    }

    return att;
}


subroutine(LightInfoType)
void computeLightInfoLOD0(in vec3 vertexWV){
    int lightType = LIGHT_DIRECTIONAL;

#if defined(COMPUTE_TBN)
    vec3 n = _normalWV;
    vec3 t = normalize(dvd_NormalMatrix * dvd_Tangent);
    vec3 b = normalize(cross(n, t));
    
    vec3 tmpVec = -vertexWV;
   _viewDirection.x = dot(tmpVec, t);
   _viewDirection.y = dot(tmpVec, b);
   _viewDirection.z = dot(tmpVec, n);
#else
    _viewDirection = -vertexWV;
#endif
    _viewDirection = normalize(_viewDirection);

    vec3 lightDirection;
    Light currentLight;

    int currentLightCount = int(ceil(dvd_lightCount / (lodLevel + 1.0f)));

    for (int i = 0; i < MAX_LIGHTS_PER_NODE; i++){
        if (currentLightCount == i) break;

        currentLight = dvd_LightSource[dvd_lightIndex[i]];
        lightType = dvd_lightType[dvd_lightIndex[i]];
        //Directional light => lightType == 0; Spot or Omni => lightType == 1
        if (lightType == LIGHT_DIRECTIONAL){
            //lightPosMV.w will be 0 for Directional Lights and 1 for Spot or Omni, so this avoids an "if/else"
            lightDirection = -normalize(currentLight._position.xyz);
            //either _attenuation == 1 if light is directional or we compute the actual value for omni and spot
            _lightInfo[i]._attenuation = 1.0;
        }else{
            //lightPosMV.w will be 0 for Directional Lights and 1 for Spot or Omni, so this avoids an "if/else"
            lightDirection = normalize(_viewDirection + currentLight._position.xyz);
            //either _attenuation == 1 if light is directional or we compute the actual value for omni and spot
            _lightInfo[i]._attenuation = computeAttenuation(currentLight, lightDirection, lightType);
        }

#if defined(COMPUTE_TBN)
        _lightInfo[i]._lightDirection.x = dot(lightDirection, t);
        _lightInfo[i]._lightDirection.y = dot(lightDirection, b);
        _lightInfo[i]._lightDirection.z = dot(lightDirection, n);
        _lightInfo[i]._lightDirection = normalize(_lightInfo[i]._lightDirection);
#else
        _lightInfo[i]._lightDirection = lightDirection;
#endif
    }
}

subroutine(LightInfoType)
void computeLightInfoLOD1(in vec3 vertexWV){
    int lightType = LIGHT_DIRECTIONAL;

    _viewDirection = normalize(-vertexWV);

    vec3 lightDirection;
    Light currentLight;

    int currentLightCount = int(ceil(dvd_lightCount / (lodLevel + 1.0f)));

    for (int i = 0; i < MAX_LIGHTS_PER_NODE; i++){
        if (currentLightCount == i) break;

        currentLight = dvd_LightSource[dvd_lightIndex[i]];

        currentLight = dvd_LightSource[dvd_lightIndex[i]];
        lightType = dvd_lightType[dvd_lightIndex[i]];
        //Directional light => lightType == 0; Spot or Omni => lightType == 1
        if (lightType == LIGHT_DIRECTIONAL){
            //lightPosMV.w will be 0 for Directional Lights and 1 for Spot or Omni, so this avoids an "if/else"
            lightDirection = -normalize(currentLight._position.xyz);
            //either _attenuation == 1 if light is directional or we compute the actual value for omni and spot
            _lightInfo[i]._attenuation = 1.0;
        }else{
            //lightPosMV.w will be 0 for Directional Lights and 1 for Spot or Omni, so this avoids an "if/else"
            lightDirection = normalize(_viewDirection + currentLight._position.xyz);
            //either _attenuation == 1 if light is directional or we compute the actual value for omni and spot
            _lightInfo[i]._attenuation = computeAttenuation(currentLight, lightDirection, lightType);
        }

        _lightInfo[i]._lightDirection = lightDirection;
    }
}

void computeLightVectors(){
    _normalWV = normalize(dvd_NormalMatrix * dvd_Normal); //<ModelView Normal 
    LightInfoRoutine(vec3(dvd_ViewMatrix * _vertexW));
}