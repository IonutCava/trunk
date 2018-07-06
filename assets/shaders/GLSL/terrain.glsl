-- Vertex
//based on: http://yannick.gerometta.free.fr/base.php?id=glsldemo

varying vec4 vPixToLightTBN[1];
varying vec3 vPixToEyeTBN;
varying vec3 vPosition;
varying vec3 vVertexMV;
varying vec3 vPositionNormalized;
varying vec3 vNormalMV;
varying vec3 vPixToLightMV;
varying vec3 vLightDirMV;
varying vec4 texCoord[2];
uniform int enable_shadow_mapping;

uniform vec3 bbox_min;
uniform vec3 bbox_max;

uniform mat4 modelViewInvMatrix;
uniform mat4 lightProjectionMatrix;
uniform mat4 modelViewProjectionMatrix;

#define LIGHT_DIRECTIONAL		0.0
#define LIGHT_OMNIDIRECTIONAL	1.0
#define LIGHT_SPOT				2.0
				
void main(void){

	gl_Position = ftransform();
	
	// Position du vertex
	vPosition = gl_Vertex.xyz;
	
	// Position du vertex si le terrain est compris entre 0.0 et 1.0
	vPositionNormalized = (gl_Vertex.xyz - bbox_min.xyz) / (bbox_max.xyz - bbox_min.xyz);
	
	texCoord[0].st = vPositionNormalized.xz;
	
	vec3 vTangent = gl_MultiTexCoord0.xyz;
	vec3 n = normalize(gl_NormalMatrix * gl_Normal);
	vec3 t = normalize(gl_NormalMatrix * vTangent);
	vec3 b = cross(n, t);
	//Get the light's position in model's space
	vec4 vLightPosMV = gl_LightSource[0].position;	
	//Get the vertex's position in model's space
	vVertexMV = vec3(gl_ModelViewMatrix * gl_Vertex);	
	if(length(vTangent) > 0){
		vNormalMV = b;
	}else{
		vNormalMV = n;
	}

	vec3 tmpVec;
	
	if(vLightPosMV.w == LIGHT_DIRECTIONAL)
		tmpVec = -vLightPosMV.xyz;	
	else
		tmpVec = vLightPosMV.xyz - vVertexMV.xyz;	

	vPixToLightMV = tmpVec;

	vPixToLightTBN[0].x = dot(tmpVec, t);
	vPixToLightTBN[0].y = dot(tmpVec, b);
	vPixToLightTBN[0].z = dot(tmpVec, n);
	// Point or directional?
	vPixToLightTBN[0].w = vLightPosMV.w;	
		
	//View vector
	tmpVec = -vVertexMV;
	vPixToEyeTBN.x = dot(tmpVec, t);
	vPixToEyeTBN.y = dot(tmpVec, b);
	vPixToEyeTBN.z = dot(tmpVec, n);
	
	
	if(length(gl_LightSource[0].spotDirection) > 0.001)	{
		// Spot light
		vLightDirMV = normalize(gl_LightSource[0].spotDirection);
		vPixToLightTBN[0].w = LIGHT_SPOT;
	}else{
		// Not a spot light
		vLightDirMV = gl_LightSource[0].spotDirection;
	}
	
	if(enable_shadow_mapping != 0) {
		// Transformed position 
		//vec4 pos = gl_ModelViewMatrix * gl_Vertex;
		// position multiplied by the inverse of the camera matrix
		//pos = modelViewInvMatrix * pos;
		// position multiplied by the light matrix. The vertex's position from the light's perspective
		texCoord[1] = lightProjectionMatrix * gl_Vertex;
	}
}

-- Fragment

//based on: http://yannick.gerometta.free.fr/base.php?id=glsldemo
varying vec4 vPixToLightTBN[1];	// Pixel to Light Vector
varying vec3 vPixToEyeTBN;		// Pixel to Eye Vector
varying vec3 vPosition;
varying vec3 vVertexMV;
varying vec3 vPositionNormalized;
varying vec3 vPixToLightMV;
varying vec3 vLightDirMV;
varying vec3 vNormalMV;
varying vec4 texCoord[2];

uniform sampler2D texDiffuseMap;
uniform sampler2D texNormalHeightMap;
uniform sampler2D texDiffuse0;
uniform sampler2D texDiffuse1;
uniform sampler2D texDiffuse2;
uniform sampler2D texDiffuse3;
uniform sampler2D texWaterCaustics;

uniform float detail_scale;
uniform float diffuse_scale;

uniform bool water_reflection_rendering;
uniform bool alphaTexture;
uniform float water_height;
uniform float time;

// Bounding Box du terrain
uniform vec3 bbox_min;

#define LIGHT_DIRECTIONAL		0.0
#define LIGHT_OMNIDIRECTIONAL	1.0
#define LIGHT_SPOT				2.0

vec4 NormalMapping(vec2 uv, vec3 vPixToEyeTBN, vec4 vPixToLightTBN, bool bParallax);
vec4 ReliefMapping(vec2 uv);
vec4 CausticsColor();
bool isUnderWater();

uniform bool enableFog;
#include "fog.frag"
#include "shadowMapping.frag"

void main (void)
{
	// Discard the fragments that are underwater when drawing in reflection
	if(water_reflection_rendering && isUnderWater()){
		discard;
	}
	
	vec4 vPixToLightTBNcurrent = vPixToLightTBN[0];
	
	vec4 color = NormalMapping(texCoord[0].st, vPixToEyeTBN, vPixToLightTBNcurrent, false);

	if(enableFog){
		color = applyFog(color);
	}

	if(isUnderWater()) {
		float alpha = (water_height - vPosition.y) / (2*(water_height - bbox_min.y));
		color = (1-alpha) * color + alpha * CausticsColor();
	}
	gl_FragColor = color;
}

bool isUnderWater(){
	return vPosition.y < water_height;
}

vec4 CausticsColor()
{
	vec2 uv0 = texCoord[0].st*100.0;
	uv0.s -= time*0.1;
	uv0.t += time*0.1;
	vec4 color0 = texture2D(texWaterCaustics, uv0);
	
	vec2 uv1 = texCoord[0].st*100.0;
	uv1.s += time*0.1;
	uv1.t += time*0.1;	
	vec4 color1 = texture2D(texWaterCaustics, uv1);
	
	return (color0 + color1) /2;	
}



vec4 NormalMapping(vec2 uv, vec3 vPixToEyeTBN, vec4 vPixToLightTBN, bool bParallax)
{	
	vec3 lightVecTBN = normalize(vPixToLightTBN.xyz);
	vec3 viewVecTBN = normalize(vPixToEyeTBN);

	vec2 uv_detail = uv * detail_scale;
	vec2 uv_diffuse = uv * diffuse_scale;

	
	vec3 normalTBN = texture2D(texNormalHeightMap, uv_detail).rgb * 2.0 - 1.0;
	normalTBN = normalize(normalTBN);
	
	vec4 tBase[3];
	vec4 alpha;
	tBase[0] = texture2D(texDiffuse0, uv_diffuse);
	tBase[1] = texture2D(texDiffuse1, uv_diffuse);	
	tBase[2] = texture2D(texDiffuse2, uv_diffuse);
	if(alphaTexture){
		alpha = texture2D(texDiffuse3, uv_diffuse);
	}
	vec4 DiffuseMap = texture2D(texDiffuseMap, uv);
	
	vec4 cBase;

	if(vPosition.y < water_height)
		cBase = tBase[0];
	else {
		if(alphaTexture){
			cBase = mix(mix(mix(tBase[1], tBase[0], DiffuseMap.r), tBase[2], DiffuseMap.g), alpha, DiffuseMap.a);
		}else{
			cBase = mix(mix(tBase[1], tBase[0], DiffuseMap.r), tBase[2], DiffuseMap.g);			
		}
	}


	float iDiffuse = max(dot(lightVecTBN.xyz, normalTBN), 0.0);	// Intensité diffuse
	float iSpecular = 0;
	
	if(isUnderWater())
		iSpecular = pow(clamp(dot(reflect(-lightVecTBN.xyz, normalTBN), viewVecTBN), 0.0, 1.0), gl_FrontMaterial.shininess )/2.0;
	
	vec4 cAmbient = gl_LightSource[0].ambient * gl_FrontMaterial.ambient;
	vec4 cDiffuse = gl_LightSource[0].diffuse * gl_FrontMaterial.diffuse * iDiffuse;	
	vec4 cSpecular = gl_LightSource[0].specular * gl_FrontMaterial.specular * iSpecular;
	
	float shadow = 1.0;
	if(enable_shadow_mapping != 0) {
		/////////////////////////
		// SHADOW MAPS
		vec3 vPixPosInDepthMap;
		//Compute shadow value for current fragment
		shadow = ShadowMapping(vPixPosInDepthMap);
		//And add shadow value to current diffuse color and specular values
		cDiffuse = (shadow) * cDiffuse;
		cSpecular = (shadow) * cSpecular;
		// Texture projection :
		if(enable_shadow_mapping == 2) {
			vec4 cProjected = texture2D(texDiffuseProjected, vec2(vPixPosInDepthMap.s, 1.0-vPixPosInDepthMap.t));
			cDiffuse.xyz = mix(cDiffuse.xyz, cProjected.xyz, shadow/2.0);
		}
	}
	

	return cAmbient * cBase + cDiffuse * cBase + cSpecular;
}