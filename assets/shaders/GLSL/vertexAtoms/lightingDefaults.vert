#define MODE_BUMP		1
#define MODE_PARALLAX	2
#define MODE_RELIEF		3

#include "lightInput.cmn"

const float pidiv180 = 0.0174532777777778; //3.14159 / 180.0; // 36 degrees

vec4 _vertexWV;
void loopNormalLOD(in vec3 T, in vec3 B) {

    vec3 lightDirection; 
    float distance = 1.0;
    float lightType = 0.0;
    Light currentLight;

    int currentLightCount = int(ceil(dvd_lightCount / (lodLevel+1.0f)));

    for (int i = 0; i < MAX_LIGHTS_PER_NODE; i++){
        if(currentLightCount == i) break;

        currentLight = dvd_LightSource[dvd_lightIndex[i]];

        //Directional light => lightType == 0; Spot or Omni => lightType == 1
        lightType = clamp(currentLight._position.w, 0.0, 1.0);

        //lightPosMV.w will be 0 for Directional Lights and 1 for Spot or Omni, so this avoids an "if/else"
        lightDirection = mix(-currentLight._position.xyz, normalize(_vertexWV.xyz - currentLight._position.xyz), lightType);

        distance = length(lightDirection);
        //either _attenuation == 1 if light is directional or we compute the actual value for omni and spot
        _lightInfo[i]._attenuation = mix(1.0, max(1.0 / (currentLight._attenuation.x + currentLight._attenuation.y * distance + currentLight._attenuation.z * distance * distance), 0.0), lightType);
        // spotlight
        if (currentLight._position.w > 1.5){
            float clampedCosine = max(0.0, dot(-lightDirection, normalize(currentLight._direction.rgb)));
            if (clampedCosine < cos(currentLight._attenuation.w * pidiv180)){ // outside of spotlight cone
                _lightInfo[i]._attenuation = 0.0;
            }else{
                _lightInfo[i]._attenuation = _lightInfo[i]._attenuation * pow(clampedCosine, currentLight._direction.w);
            }
        }

#if defined(COMPUTE_TBN)
        _lightInfo[i]._lightDirection = vec3(dot(lightDirection, T), dot(lightDirection, B), dot(lightDirection, _normalWV));
#else
        _lightInfo[i]._lightDirection = lightDirection;
#endif
    }
}

void loopLowLOD() {
    vec3 lightDirection; 

    float distance = 1.0;
    float lightType = 0.0;
    Light currentLight;

    int currentLightCount = int(ceil(dvd_lightCount / (lodLevel+1.0f)));

    for (int i = 0; i < MAX_LIGHTS_PER_NODE; i++){
        if(currentLightCount == i) break;

        currentLight = dvd_LightSource[dvd_lightIndex[i]];

        //Directional light => lightType == 0; Spot or Omni => lightType == 1
        lightType = clamp(currentLight._position.w, 0.0, 1.0);

        //lightPosMV.w will be 0 for Directional Lights and 1 for Spot or Omni, so this avoids an "if/else"
        lightDirection = mix(-currentLight._position.xyz, normalize(_vertexWV.xyz - currentLight._position.xyz), lightType);

        distance = length(lightDirection);
        //either _attenuation == 1 if light is directional or we compute the actual value for omni and spot
        _lightInfo[i]._attenuation = mix(1.0, max(1.0 / (currentLight._attenuation.x + currentLight._attenuation.y * distance + currentLight._attenuation.z * distance * distance), 0.0), lightType);

        // spotlight
        if (currentLight._position.w > 1.5){
            float clampedCosine = max(0.0, dot(-lightDirection, normalize(currentLight._direction.rgb)));
            if (clampedCosine < cos(currentLight._attenuation.w * pidiv180)){ // outside of spotlight cone
                _lightInfo[i]._attenuation = 0.0;
            }else{
                _lightInfo[i]._attenuation = _lightInfo[i]._attenuation * pow(clampedCosine, currentLight._direction.w);
            }
        }

        _lightInfo[i]._lightDirection = lightDirection;
    }
}

void computeLightVectors(){
    _vertexWV = dvd_ViewMatrix * _vertexW; 	      //<ModelView Vertex  
    _normalWV = normalize(dvd_NormalMatrix * dvd_Normal); //<ModelView Normal 

    #if defined(COMPUTE_TBN)
        if (lodLevel == 0) {
            vec3 T = normalize(dvd_NormalMatrix * dvd_Tangent);
            vec3 B = cross(_normalWV, T);

            if(length(dvd_Tangent) > 0){
                _normalWV = B;
            }

            _viewDirection  = vec3(dot(-_vertexWV.xyz, T), dot(-_vertexWV.xyz, B), dot(-_vertexWV.xyz, _normalWV));
            loopNormalLOD(T, B);
        }else{
            _viewDirection = -_vertexWV.xyz;
            loopLowLOD();
        }
    #else
        _viewDirection = -_vertexWV.xyz;
        loopLowLOD();
    #endif
}