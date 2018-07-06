-- Vertex
#include "vboInputData.vert"
#include "foliage.vert"

uniform bool enable_shadow_mapping;

varying vec3 vPixToLightTBN[MAX_LIGHT_COUNT];
uniform mat4 lightProjectionMatrices[MAX_SHADOW_CASTING_LIGHTS];
varying vec4 shadowCoord[MAX_SHADOW_CASTING_LIGHTS];
varying vec3 _normalMV;
varying vec4 _vertexMV;

uniform int light_count;

void computeLightVectorsPhong(){
	vec3 tmpVec; 

	int i = 0; ///Only the first light for now
	//for(int i = 0; i < MAX_LIGHT_COUNT; i++){
	//	if(light_count == i) break;
	vec4 vLightPosMV = gl_LightSource[i].position;	
	if(vLightPosMV.w == 0.0){ ///<Directional Light
		tmpVec = -vLightPosMV.xyz;					
	}else{///<Omni or spot. Change later if spot
		tmpVec = vLightPosMV.xyz - _vertexMV.xyz;	
	}
	vPixToLightTBN[0] = tmpVec;
}

void main(void){
	computeData();
	
	_normalMV = gl_NormalMatrix * normalData;

	computeFoliageMovementGrass(normalData, _normalMV, vertexData);
	_vertexMV = gl_ModelViewMatrix * vertexData;

	computeLightVectorsPhong();
	vec3 vLightPosMVTemp = vPixToLightTBN[0];
	float intensity = dot(vLightPosMVTemp.xyz, _normalMV);
	gl_FrontColor = vec4(intensity, intensity, intensity, 1.0);
	gl_FrontColor.a = 1.0 - clamp(length(_vertexMV)/lod_metric, 0.0, 1.0);
		
	
	gl_Position = projectionMatrix * _vertexMV;
	
	if(enable_shadow_mapping) {
		// position multiplied by the light matrix. 
		//The vertex's position from the light's perspective
		shadowCoord[0] = lightProjectionMatrices[0] * modelViewInvMatrix * _vertexMV;
	}
}

-- Fragment

varying vec2 _texCoord;
varying vec3 _normalMV;
varying vec4 _vertexMV;
varying vec3 vPixToLightTBN[MAX_LIGHT_COUNT];
uniform sampler2D texDiffuse;

///Global NDotL, basically
float iDiffuse;
#include "shadowMapping.frag"

void main (void){

	vec4 cBase = texture(texDiffuse, _texCoord);
	if(cBase.a < 0.4) discard;
	
	vec4 cAmbient = gl_LightSource[0].ambient;
	vec4 cDiffuse = gl_LightSource[0].diffuse * gl_Color;
	vec3 L = normalize(vPixToLightTBN[0]);
	iDiffuse = max(dot(L, _normalMV), 0.0);
	// SHADOW MAPPING
	vec3 vPixPosInDepthMap;
	float shadow = 1.0f;
	applyShadowDirectional(0, shadow);

	gl_FragColor = cAmbient * cBase + (0.2 + 0.8 * shadow) * cDiffuse * cBase;
	
	gl_FragColor.a = gl_Color.a;
}


