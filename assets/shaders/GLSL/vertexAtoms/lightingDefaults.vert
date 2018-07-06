#include "lightInput.cmn"

subroutine void LightInfoType(in vec3 vertexWV);
subroutine uniform LightInfoType LightInfoRoutine;

const float pidiv180 = 0.0174532777777778; //3.14159 / 180.0; // 36 degrees

float computeAttenuation(const in uint currentLightIndex, const in vec3 lightDirection, const in uint lightType){
    float distance = length(lightDirection);

    vec4 attIn = dvd_LightSourcePhysical[currentLightIndex]._attenuation;
    vec4 dirIn = dvd_LightSourcePhysical[currentLightIndex]._direction;

    float att = max(1.0 / (attIn.x + (attIn.y * distance) + (attIn.z * distance * distance)), 0.0);

    if (lightType == LIGHT_SPOT){
        float clampedCosine = max(0.0, dot(-lightDirection, normalize(dirIn.xyz)));
        return mix(0.0, att * pow(clampedCosine, dirIn.w), clampedCosine < cos(dvd_LightSourcePhysical[currentLightIndex]._spotCutoff * pidiv180));
    }

    return att;
}

subroutine(LightInfoType)
void computeLightInfoLOD0(in vec3 vertexWV){
    uint lightType = LIGHT_DIRECTIONAL;

#if defined(COMPUTE_TBN)
    vec3 n = _normalWV;
    vec3 t = normalize(dvd_NormalMatrix[dvd_drawID] * dvd_Tangent);
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

    _lightInfo._lightCount = uint(ceil(dvd_perNodeLightData[0].w / (lodLevel + 1.0f)));

    for (uint i = 0; i < MAX_LIGHTS_PER_NODE; i++){
        if (_lightInfo._lightCount == i) break;

        uint currentLightIndex = dvd_perNodeLightData[i].x;
        
        lightType = dvd_perNodeLightData[currentLightIndex].y;
        //Directional light => lightType == 0; Spot or Omni => lightType == 1
        if (lightType == LIGHT_DIRECTIONAL){
            //lightPosMV.w will be 0 for Directional Lights and 1 for Spot or Omni, so this avoids an "if/else"
            lightDirection = -normalize(dvd_LightSourcePhysical[currentLightIndex]._position.xyz);
            //either _attenuation == 1 if light is directional or we compute the actual value for omni and spot
            _lightInfo._attenuation[i] = 1.0;
        }else{
            //lightPosMV.w will be 0 for Directional Lights and 1 for Spot or Omni, so this avoids an "if/else"
            lightDirection = normalize(_viewDirection + dvd_LightSourcePhysical[currentLightIndex]._position.xyz);
            //either _attenuation == 1 if light is directional or we compute the actual value for omni and spot
            _lightInfo._attenuation[i] = computeAttenuation(currentLightIndex, lightDirection, lightType);
        }

#if defined(COMPUTE_TBN)
        _lightInfo._lightDirection[i] = normalize(vec3(dot(lightDirection, t), dot(lightDirection, b), dot(lightDirection, n)));
#else
        _lightInfo._lightDirection[i] = lightDirection;
#endif
    }
}

subroutine(LightInfoType)
void computeLightInfoLOD1(in vec3 vertexWV){
    uint lightType = LIGHT_DIRECTIONAL;

    _viewDirection = normalize(-vertexWV);

    vec3 lightDirection;

    _lightInfo._lightCount = uint(ceil(dvd_perNodeLightData[0].w / (lodLevel + 1.0f)));

    for (uint i = 0; i < MAX_LIGHTS_PER_NODE; i++){
        if (_lightInfo._lightCount == i) break;

        uint currentLightIndex = dvd_perNodeLightData[i].x;

        lightType = dvd_perNodeLightData[currentLightIndex].y;
        //Directional light => lightType == 0; Spot or Omni => lightType == 1
        if (lightType == LIGHT_DIRECTIONAL){
            //lightPosMV.w will be 0 for Directional Lights and 1 for Spot or Omni, so this avoids an "if/else"
            lightDirection = -normalize(dvd_LightSourcePhysical[currentLightIndex]._position.xyz);
            //either _attenuation == 1 if light is directional or we compute the actual value for omni and spot
            _lightInfo._attenuation[i] = 1.0;
        }else{
            //lightPosMV.w will be 0 for Directional Lights and 1 for Spot or Omni, so this avoids an "if/else"
            lightDirection = normalize(_viewDirection + dvd_LightSourcePhysical[currentLightIndex]._position.xyz);
            //either _attenuation == 1 if light is directional or we compute the actual value for omni and spot
            _lightInfo._attenuation[i] = computeAttenuation(currentLightIndex, lightDirection, lightType);
        }

        _lightInfo._lightDirection[i] = lightDirection;
    }
}

void computeLightVectors(){
    _normalWV = normalize(dvd_NormalMatrix[dvd_drawID] * dvd_Normal); //<ModelView Normal 
    LightInfoRoutine(vec3(dvd_ViewMatrix * _vertexW));
}