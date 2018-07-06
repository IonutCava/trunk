#define MODE_BUMP		1
#define MODE_PARALLAX	2
#define MODE_RELIEF		3

out vec3 _lightDirection[MAX_LIGHT_COUNT]; //<Light direction
out vec3 _viewDirection[MAX_LIGHT_COUNT];
out vec4 _shadowCoord[MAX_SHADOW_CASTING_LIGHTS];
out float _attenuation[MAX_LIGHT_COUNT];

out vec4  _vertexM;
out vec4  _vertexMV;
out vec3  _normalMV;

uniform bool dvd_enableShadowMapping;
uniform int  dvd_lightCount;
uniform mat4 dvd_lightProjectionMatrices[MAX_SHADOW_CASTING_LIGHTS];

const float pidiv180 = 3.14159 / 180.0; // 36 degrees

void computeLightVectors(){

	_normalMV = normalize(dvd_NormalMatrix * dvd_Normal); //<ModelView Normal 

#if defined(COMPUTE_TBN)
	vec3 T = normalize(dvd_NormalMatrix * dvd_Tangent);
	vec3 B = cross(_normalMV, T);

    if(length(dvd_Tangent) > 0){
		_normalMV = B;
	}
#endif

    _vertexMV = dvd_ModelViewMatrix * dvd_Vertex; 	   //< ModelView Vertex  
	_vertexM = dvd_ModelMatrix * dvd_Vertex;
    vec3 lightDirection; 
    vec3 lightPosMV;
    float distance = 1.0;
    float lightType = 0.0;
	for(int i = 0; i < MAX_LIGHT_COUNT; i++){
		if(dvd_lightCount == i) break;

		lightPosMV = vec3(dvd_ModelViewMatrix * gl_LightSource[i].position);	
        lightType = gl_LightSource[i].position.w;

        //lightPosMV.w will be 0 for Directional Lights and 1 for Spot or Omni, so this avoids an "if/else"
		lightDirection = normalize(lightPosMV - (_vertexMV.xyz  * lightType));
        if(lightType == 0.0) lightDirection = -lightDirection;
        distance = length(lightDirection);
        //either _attenuation == 1 if light is directional or we compute the actual value for omni and spot
        _attenuation[i] = mix(1.0,max(1.0/(gl_LightSource[i].constantAttenuation + 
		                                   gl_LightSource[i].linearAttenuation * distance + 
					                       gl_LightSource[i].quadraticAttenuation * distance * distance),
                                      0.0),//max
                              lightType);//mix
        // spotlight
        if (gl_LightSource[i].spotCutoff <= 90.0){
            float clampedCosine = max(0.0, dot(-lightDirection, normalize(gl_LightSource[i].spotDirection)));
            if (clampedCosine < cos(gl_LightSource[i].spotCutoff * pidiv180)){ // outside of spotlight cone
                _attenuation[i] = 0.0;
            }else{
                _attenuation[i] = _attenuation[i] * pow(clampedCosine, gl_LightSource[i].spotExponent);
            }
        }
#if defined(COMPUTE_TBN)
        _lightDirection[i].x = dot(lightDirection, T);
		_lightDirection[i].y = dot(lightDirection, B);
		_lightDirection[i].z = dot(lightDirection, _normalMV);
		_viewDirection[i].x = dot(-_vertexMV.xyz, T);
		_viewDirection[i].y = dot(-_vertexMV.xyz, B);
		_viewDirection[i].z = dot(-_vertexMV.xyz, _normalMV);
#else
        _lightDirection[i] = lightDirection;
    	_viewDirection[i] = -_vertexMV.xyz;
#endif

	}


	if(dvd_enableShadowMapping) {
		// position multiplied by the inverse of the camera matrix
		// position multiplied by the light matrix. The vertex's position from the light's perspective
		for(int i = 0; i < MAX_SHADOW_CASTING_LIGHTS; i++){
			_shadowCoord[i] = dvd_lightProjectionMatrices[i] * _vertexM;
		}
	}	
}