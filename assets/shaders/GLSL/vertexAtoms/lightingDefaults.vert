#include "lightInput.cmn"

out vec4 _vertexWV;

subroutine void LightInfoType();
subroutine uniform LightInfoType LightInfoRoutine;

const float pidiv180 = 0.0174532777777778; //3.14159 / 180.0; // 36 degrees

float computeAttenuation(const in uint currentLightIndex, const in vec3 lightDirection, const in uint lightType){
    float distance = length(lightDirection);

    vec4 attIn = dvd_LightSource[currentLightIndex]._attenuation;
    vec4 dirIn = dvd_LightSource[currentLightIndex]._direction;

    float att = max(1.0 / (attIn.x + (attIn.y * distance) + (attIn.z * distance * distance)), 0.0);

    if (lightType == LIGHT_SPOT){
        float clampedCosine = max(0.0, dot(-lightDirection, normalize(dirIn.xyz)));
        return mix(0.0, att * pow(clampedCosine, dirIn.w), clampedCosine < cos(dvd_LightSource[currentLightIndex]._specular.w * pidiv180));
    }

    return att;
}

subroutine(LightInfoType)
void computeLightInfoLOD0(){
    uint lightType = LIGHT_DIRECTIONAL;

#if defined(COMPUTE_TBN)
    vec3 n = _normalWV;
    vec3 t = normalize(dvd_NormalMatrix * dvd_Tangent);
    vec3 b = normalize(cross(n, t));
    
    vec3 tmpVec = -_vertexWV.xyz;
   _viewDirection.x = dot(tmpVec, t);
   _viewDirection.y = dot(tmpVec, b);
   _viewDirection.z = dot(tmpVec, n);
#else
    _viewDirection = -_vertexWV.xyz;
#endif
    _viewDirection = normalize(_viewDirection);

    vec3 lightDirection;

    for (uint i = 0; i < MAX_LIGHTS_PER_SCENE; i++){
        if (_lightCount == i) break;
        
        lightType = uint(dvd_LightSource[i]._position.w);
        //Directional light => lightType == 0; Spot or Omni => lightType == 1
        if (lightType == LIGHT_DIRECTIONAL){
            //lightPosMV.w will be 0 for Directional Lights and 1 for Spot or Omni, so this avoids an "if/else"
            lightDirection = -normalize(dvd_LightSource[i]._position.xyz);
            //either _attenuation == 1 if light is directional or we compute the actual value for omni and spot
            _lightInfo[i]._attenuation = 1.0;
        }else{
            //lightPosMV.w will be 0 for Directional Lights and 1 for Spot or Omni, so this avoids an "if/else"
            lightDirection = normalize(_viewDirection + dvd_LightSource[i]._position.xyz);
            //either _attenuation == 1 if light is directional or we compute the actual value for omni and spot
            _lightInfo[i]._attenuation = computeAttenuation(i, lightDirection, lightType);
        }

#if defined(COMPUTE_TBN)
        _lightInfo[i]._lightDirection = normalize(vec3(dot(lightDirection, t), dot(lightDirection, b), dot(lightDirection, n)));
#else
        _lightInfo[i]._lightDirection = lightDirection;
#endif
    }
}

subroutine(LightInfoType)
void computeLightInfoLOD1(){
    uint lightType = LIGHT_DIRECTIONAL;

    _viewDirection = normalize(-_vertexWV.xyz);

    vec3 lightDirection;

    for (uint i = 0; i < MAX_LIGHTS_PER_SCENE; i++){
        if (_lightCount == i) break;

        lightType = uint(dvd_LightSource[i]._position.w);
        //Directional light => lightType == 0; Spot or Omni => lightType == 1
        if (lightType == LIGHT_DIRECTIONAL){
            //lightPosMV.w will be 0 for Directional Lights and 1 for Spot or Omni, so this avoids an "if/else"
            lightDirection = -normalize(dvd_LightSource[i]._position.xyz);
            //either _attenuation == 1 if light is directional or we compute the actual value for omni and spot
            _lightInfo[i]._attenuation = 1.0;
        }else{
            //lightPosMV.w will be 0 for Directional Lights and 1 for Spot or Omni, so this avoids an "if/else"
            lightDirection = normalize(_viewDirection + dvd_LightSource[i]._position.xyz);
            //either _attenuation == 1 if light is directional or we compute the actual value for omni and spot
            _lightInfo[i]._attenuation = computeAttenuation(i, lightDirection, lightType);
        }

        _lightInfo[i]._lightDirection = lightDirection;
    }
}

void computeLightVectors(){
    _lightCount = dvd_lightCount;
    _normalWV = normalize(dvd_NormalMatrix * dvd_Normal); //<ModelView Normal 
	_vertexWV = dvd_ViewMatrix * _vertexW;
    LightInfoRoutine();
}