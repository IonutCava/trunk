-- Vertex

#include "vboInputData.vert"
#include "foliage.vert"

uniform bool dvd_enableShadowMapping;
uniform mat4 dvd_lightProjectionMatrices[MAX_SHADOW_CASTING_LIGHTS];

out vec3 _lightDirection[MAX_LIGHT_COUNT];
out vec4 _shadowCoord[MAX_SHADOW_CASTING_LIGHTS];
out vec3 _normalMV;
out vec4 _vertexMV;

uniform int dvd_lightCount;

void computeLightVectorsPhong(){
	vec3 tmpVec; 

	int i = 0; ///Only the first light for now
	//for(int i = 0; i < MAX_LIGHT_COUNT; i++){
	//	if(dvd_lightCount == i) break;
	vec4 vLightPosMV = gl_LightSource[i].position;	
	if(vLightPosMV.w == 0.0){ ///<Directional Light
		tmpVec = -vLightPosMV.xyz;					
	}else{///<Omni or spot. Change later if spot
		tmpVec = vLightPosMV.xyz - _vertexMV.xyz;	
	}
	_lightDirection[0] = tmpVec;
}

void main(void){
	computeData();
	
	_normalMV = normalize(dvd_NormalMatrix * dvd_Normal);

	computeFoliageMovementGrass(dvd_Normal, _normalMV, dvd_Vertex);
	 vec4 vertexM = dvd_ModelMatrix * dvd_Vertex;
    _vertexMV = dvd_ViewMatrix * vertexM; 	   //< ModelView Vertex  

	computeLightVectorsPhong();
	vec3 vLightPosMVTemp = _lightDirection[0];
	float intensity = dot(vLightPosMVTemp.xyz, _normalMV);
	gl_FrontColor = vec4(intensity, intensity, intensity, 1.0);
	gl_FrontColor.a = 1.0 - clamp(length(_vertexMV)/lod_metric, 0.0, 1.0);
		
	
	gl_Position = dvd_ProjectionMatrix * _vertexMV;
	
	if(dvd_enableShadowMapping) {
		// position multiplied by the light matrix. 
		//The vertex's position from the light's perspective
		_shadowCoord[0] = dvd_lightProjectionMatrices[0] * vertexM;
	}
}

-- Fragment

in vec2 _texCoord;
in vec3 _normalMV;
in vec4 _vertexMV;
in vec3 _lightDirection[MAX_LIGHT_COUNT];
out vec4 _colorOut;

uniform sampler2D texDiffuse;

///Global NDotL, basically
float iDiffuse;
#include "shadowMapping.frag"

void main (void){

	vec4 cBase = texture(texDiffuse, _texCoord);
	if(cBase.a < ALPHA_DISCARD_THRESHOLD + 0.1) discard;
	
	vec4 cAmbient = gl_LightSource[0].ambient;
	vec4 cDiffuse = gl_LightSource[0].diffuse * gl_Color;
	vec3 L = normalize(_lightDirection[0]);
	iDiffuse = max(dot(L, _normalMV), 0.0);
	// SHADOW MAPPING
	vec3 vPixPosInDepthMap;
	float shadow = 1.0;
	applyShadowDirectional(iDiffuse, 0, shadow);

	_colorOut = cAmbient * cBase + (0.2 + 0.8 * shadow) * cDiffuse * cBase;
	
	_colorOut.a = gl_Color.a;
}


