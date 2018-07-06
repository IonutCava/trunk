varying vec3 vPixToLightTBN[MAX_LIGHT_COUNT];
varying vec3 vPixToLightMV[MAX_LIGHT_COUNT];
varying vec3 vPixToEyeTBN[MAX_LIGHT_COUNT];
varying vec4 shadowCoord[MAX_SHADOW_CASTING_LIGHTS];

varying vec4 _vertexMV;
varying vec3 _normalMV;

uniform bool enable_shadow_mapping;
uniform int light_count;
uniform mat4 lightProjectionMatrices[MAX_SHADOW_CASTING_LIGHTS];

#include "lightingDefaultsBump.vert"

void computeLightVectorsPhong(){
	vec3 tmpVec; 

	for(int i = 0; i < MAX_LIGHT_COUNT; i++){
		if(light_count == i) break;
		vec4 vLightPosMV = gl_LightSource[i].position;	
		if(vLightPosMV.w == 0.0){ ///<Directional Light
			tmpVec = -vLightPosMV.xyz;					
		}else{///<Omni or spot. Change later if spot
			tmpVec = vLightPosMV.xyz - _vertexMV.xyz;	
		}

		vPixToLightMV[i] = tmpVec;
		vPixToLightTBN[i] = tmpVec;
		vPixToEyeTBN[i] = -_vertexMV.xyz;
	}
}


void computeLightVectors(in bool bump){

	vec3 n = normalize(gl_NormalMatrix * normalData);
	vec3 t = normalize(gl_NormalMatrix * tangentData);
	vec3 b = cross(n, t);
	_normalMV = n;
	// Transformed position 
	_vertexMV = gl_ModelViewMatrix * vertexData;
	
	if(bump == false){
		computeLightVectorsPhong();
	}else{
		computeLightVectorsBump(t,b,n);
	}

	if(enable_shadow_mapping) {
		// position multiplied by the inverse of the camera matrix
		// position multiplied by the light matrix. The vertex's position from the light's perspective
		for(int i = 0; i < MAX_SHADOW_CASTING_LIGHTS; i++){
			shadowCoord[i] = lightProjectionMatrices[i] * modelViewInvMatrix * _vertexMV;
		}
	}	
}