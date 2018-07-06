-- Vertex
#include "vboInputData.vert"
varying vec4 texCoord[2];
varying vec3 vVertexMV;

uniform float time;
uniform float lod_metric;
uniform float scale;
uniform float windDirectionX;
uniform float windDirectionZ;
uniform float windSpeed;
uniform int enable_shadow_mapping;
uniform mat4 modelViewInvMatrix;
uniform mat4 lightProjectionMatrix;

void main(void){
	computeData();
	texCoord[0] = vec4(texCoordData,0,0);
	
	vec4 vertex = vertexData;
	vec4 vertexMV = gl_ModelViewMatrix * vertexData;
	vec3 normalMV = gl_NormalMatrix * normalData;
	vVertexMV = vertexMV.xyz;

	if(normalData.y < 0.0 ) {
		normalMV = -normalMV;
		vertex.x += ((0.5*scale)*cos(time*windSpeed) * cos(vertex.x) * sin(vertex.x))*windDirectionX;
		vertex.z += ((0.5*scale)*sin(time*windSpeed) * cos(vertex.x) * sin(vertex.x))*windDirectionZ;
	}
	
	vec4 vLightPosMV = -gl_LightSource[0].position;	
	float intensity = dot(vLightPosMV.xyz, normalMV);
	gl_FrontColor = vec4(intensity, intensity, intensity, 1.0);
	gl_FrontColor.a = 1.0 - clamp(length(vertexMV)/lod_metric, 0.0, 1.0);
		
	gl_Position = gl_ModelViewProjectionMatrix * vertex;
	
	if(enable_shadow_mapping != 0) {
		// Transformed position 
		vec4 pos = gl_ModelViewMatrix * vertexData;
		// position multiplied by the inverse of the camera matrix
		pos = modelViewInvMatrix * pos;
		// position multiplied by the light matrix. 
		//The vertex's position from the light's perspective
		texCoord[1] = lightProjectionMatrix * pos;
	}
}

-- Fragment

varying vec4 texCoord[2];
varying vec3 vVertexMV;

uniform sampler2D texDiffuse;

// SHADOW MAPPING //
// 0->no  1->shadow mapping  2->shadow mapping + projected texture
uniform int enable_shadow_mapping;
//tdmfl0 -> high detail
//tdmfl1 -> medium detail
//tdmfl2 -> low detail
//change according to distance from eye
uniform float resolutionFactor;
uniform sampler2DShadow texDepthMapFromLight0;
uniform sampler2DShadow texDepthMapFromLight1;
uniform sampler2DShadow texDepthMapFromLight2;
#define Z_TEST_SIGMA 0.0001
////////////////////

float ShadowMapping(out vec3 vPixPosInDepthMap);
float filterFinalShadow(sampler2DShadow depthMap,vec3 vPosInDM, float resolution);

void main (void)
{
	vec4 cBase = texture2D(texDiffuse, texCoord[0].st);
	if(cBase.a < 0.4) discard;
	
	vec4 cAmbient = gl_LightSource[0].ambient * gl_FrontMaterial.ambient;
	vec4 cDiffuse = gl_LightSource[0].diffuse * gl_FrontMaterial.diffuse * gl_Color;
	gl_FragColor = cAmbient * cBase + cDiffuse * cBase;
	
	// SHADOW MAPPING
	vec3 vPixPosInDepthMap;
	float shadow = ShadowMapping(vPixPosInDepthMap);
	shadow = shadow * 0.5 + 0.5;
	gl_FragColor *= shadow;

	
	gl_FragColor.a = gl_Color.a;
}


float ShadowMapping(out vec3 vPixPosInDepthMap){

	float fShadow = 1.0;
			
	float tOrtho[3];
	tOrtho[0] = 5.0;
	tOrtho[1] = 10.0;
	tOrtho[2] = 50.0;
	bool ok = false;
	int id = 0;
	vec3 posInDM;
	for(int i = 0; i < 3; i++){
		if(!ok){
		
			vPixPosInDepthMap = vec3(texCoord[1].xy/tOrtho[i], texCoord[1].z) / (texCoord[1].w);
			vPixPosInDepthMap = (vPixPosInDepthMap + 1.0) * 0.5;

			if(vPixPosInDepthMap.x >= 0.0 && vPixPosInDepthMap.y >= 0.0 && vPixPosInDepthMap.x <= 1.0 && vPixPosInDepthMap.y <= 1.0){
				id = i;
				ok = true;
				posInDM = vPixPosInDepthMap;
				break; // no need to continue
			}
		}
	}

	if(ok){
		switch(id){
			case 0:
				fShadow = filterFinalShadow(texDepthMapFromLight0,posInDM, 2048/resolutionFactor);
				break;
			case 1:
				fShadow = filterFinalShadow(texDepthMapFromLight1,posInDM, 1024/resolutionFactor);
				break;
			case 2:
				fShadow = filterFinalShadow(texDepthMapFromLight2,posInDM, 512/resolutionFactor);
				break;
		};
	}
	
	return fShadow;
}

float filterFinalShadow(sampler2DShadow depthMap,vec3 vPosInDM, float resolution){
	// Gaussian 3x3 filter
	vec4 vDepthMapColor = shadow2D(depthMap, vPosInDM);
	float fShadow = 0.0;
	if((vDepthMapColor.z+Z_TEST_SIGMA) < vPosInDM.z){
		fShadow = shadow2D(depthMap, vPosInDM).x * 0.25;
		fShadow += shadow2DOffset(depthMap, vPosInDM, ivec2( -1, -1)).x * 0.0625;
		fShadow += shadow2DOffset(depthMap, vPosInDM, ivec2( -1, 0)).x * 0.125;
		fShadow += shadow2DOffset(depthMap, vPosInDM, ivec2( -1, 1)).x * 0.0625;
		fShadow += shadow2DOffset(depthMap, vPosInDM, ivec2( 0, -1)).x * 0.125;
		fShadow += shadow2DOffset(depthMap, vPosInDM, ivec2( 0, 1)).x * 0.125;
		fShadow += shadow2DOffset(depthMap, vPosInDM, ivec2( 1, -1)).x * 0.0625;
		fShadow += shadow2DOffset(depthMap, vPosInDM, ivec2( 1, 0)).x * 0.125;
		fShadow += shadow2DOffset(depthMap, vPosInDM, ivec2( 1, 1)).x * 0.0625;

		fShadow = clamp(fShadow, 0.0, 1.0);
	}else{
		fShadow = 1.0;
	}
	return fShadow;
}
