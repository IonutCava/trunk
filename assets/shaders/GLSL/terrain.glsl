-- Vertex
//based on: http://yannick.gerometta.free.fr/base.php?id=glsldemo
#include "vboInputData.vert"

varying vec3 vPixToLightTBN[MAX_LIGHT_COUNT];
varying vec3 vPixToEyeTBN[MAX_LIGHT_COUNT];
varying vec4 shadowCoord[MAX_SHADOW_CASTING_LIGHTS];
varying vec3 vPixToLightMV[MAX_LIGHT_COUNT];

varying vec3 _normalMV;
varying vec4 _vertexMV;
uniform bool enable_shadow_mapping;

uniform vec3 bbox_min;
uniform vec3 bbox_max;

uniform mat4 lightProjectionMatrices[MAX_SHADOW_CASTING_LIGHTS];

uniform int light_count;
#include "lightingDefaultsBump.vert"

void main(void){
	int i = 0;
	computeData();
	
	// Position du vertex
	_vertexMV = vertexData;
	
	// Position du vertex si le terrain est compris entre 0.0 et 1.0
	vec3 vPositionNormalized = (vertexData.xyz - bbox_min.xyz) / (bbox_max.xyz - bbox_min.xyz);
	
	_texCoord = vPositionNormalized.xz;

	vec3 vTangent = tangentData;
	vec3 n = normalize(gl_NormalMatrix * normalData);
	vec3 t = normalize(gl_NormalMatrix * tangentData);
	vec3 b = cross(n, t);
	//Get the light's position in model's space
	vec4 vLightPosMV = gl_LightSource[0].position;	
	//Get the vertex's position in model's space
	vec3 vertexMV = vec3(gl_ModelViewMatrix * vertexData);	
	if(length(vTangent) > 0){
		_normalMV = b;
	}else{
		_normalMV = n;
	}

	computeLightVectorsBump(t,b,n);

	
	if(enable_shadow_mapping) {
		// Transformed position 
		//vec4 pos = gl_ModelViewMatrix * gl_Vertex;
		// position multiplied by the inverse of the camera matrix
		//pos = modelViewInvMatrix * pos;
		// position multiplied by the light matrix. The vertex's position from the light's perspective
		shadowCoord[0] = lightProjectionMatrices[i] * vertexData;
	}

	gl_Position = projectionMatrix * gl_ModelViewMatrix * vertexData;
}

-- Fragment

//based on: http://yannick.gerometta.free.fr/base.php?id=glsldemo
varying vec3 vPixToLightTBN[MAX_LIGHT_COUNT];
varying vec3 vPixToEyeTBN[MAX_LIGHT_COUNT];
varying vec4 shadowCoord[MAX_SHADOW_CASTING_LIGHTS];
varying vec3 vPixToLightMV[MAX_LIGHT_COUNT];

varying vec2 _texCoord;
varying vec3 _normalMV;
varying vec4 _vertexMV;

uniform int  lightType[MAX_LIGHT_COUNT];
uniform sampler2D texDiffuseMap;
uniform sampler2D texNormalHeightMap;
uniform sampler2D texDiffuse0;
uniform sampler2D texDiffuse1;
uniform sampler2D texDiffuse2;
uniform sampler2D texDiffuse3;
uniform sampler2D texWaterCaustics;

uniform float detail_scale;
uniform float diffuse_scale;
uniform int   LOD;
uniform bool water_reflection_rendering;
uniform bool alphaTexture;
uniform float water_height;
uniform float time;

// Bounding Box du terrain
uniform vec3 bbox_min;

#define LIGHT_DIRECTIONAL		0
#define LIGHT_OMNIDIRECTIONAL	1
#define LIGHT_SPOT				2

vec4 NormalMapping(vec2 uv, vec3 vPixToEyeTBN, vec3 vPixToLightTBN, bool underwater);
vec4 CausticsColor();

///Global NDotL, basically
float iDiffuse;

#include "fog.frag"
#include "shadowMapping.frag"

bool isUnderWater(){
	return _vertexMV.y < water_height;
}

void main (void)
{
	// Discard the fragments that are underwater when drawing in reflection
	bool underwater = isUnderWater();
	if(water_reflection_rendering && underwater){
		discard;
	}
	
	vec4 color = NormalMapping(_texCoord, vPixToEyeTBN[0], vPixToLightTBN[0], underwater);
	
	if(underwater) {
		float alpha = (water_height - _vertexMV.y) / (2*(water_height - bbox_min.y));
		color = (1-alpha) * color + alpha * CausticsColor();
	}

	gl_FragData[0] = applyFog(color);
}

vec4 CausticsColor()
{
	float time2 = time * 0.000001;
	vec2 uv0 = _texCoord*100.0;
	uv0.s -= time2;
	uv0.t += time2;
	vec4 color0 = texture(texWaterCaustics, uv0);
	
	vec2 uv1 = _texCoord*100.0;
	uv1.s += time2;
	uv1.t += time2;	
	vec4 color1 = texture(texWaterCaustics, uv1);
	
	return (color0 + color1) /2;	
}



vec4 NormalMapping(vec2 uv, vec3 vPixToEyeTBN, vec3 vPixToLightTBN, bool underwater)
{	
	vec3 lightVecTBN = normalize(vPixToLightTBN);
	vec3 viewVecTBN = normalize(vPixToEyeTBN);

	vec2 uv_detail = uv * detail_scale;
	vec2 uv_diffuse = uv * diffuse_scale / LOD;

	
	vec3 normalTBN = texture(texNormalHeightMap, uv_detail).rgb * 2.0 - 1.0;
	normalTBN = normalize(normalTBN);
	
	vec4 tBase[4];

	tBase[0] = texture(texDiffuse0, uv_diffuse);
	tBase[1] = texture(texDiffuse1, uv_diffuse);	
	tBase[2] = texture(texDiffuse2, uv_diffuse);
	tBase[3] = texture(texDiffuse3, uv_diffuse);
	
	vec4 DiffuseMap = texture(texDiffuseMap, uv);
	
	vec4 cBase;

	if(_vertexMV.y < water_height)
		cBase = tBase[0];
	else {
		if(alphaTexture){
			cBase = mix(mix(mix(tBase[1], tBase[0], DiffuseMap.r), tBase[2], DiffuseMap.g), tBase[3], DiffuseMap.a);
		}else{
			cBase = mix(mix(tBase[1], tBase[0], DiffuseMap.r), tBase[2], DiffuseMap.g);			
		}
	}

	// SHADOW MAPS
	float distance_max = 200.0;
	float shadow = 1.0;
	float distance = length(vPixToEyeTBN);
	if(distance < distance_max) {
		applyShadowDirectional(0, shadow);
		shadow = 1.0 - (1.0-shadow) * (distance_max-distance) / distance_max;
	}

	iDiffuse = max(dot(lightVecTBN.xyz, normalTBN), 0.0);	// diffuse intensity. NDotL

	vec4 cAmbient = gl_LightSource[0].ambient * gl_FrontMaterial.ambient;
	vec4 cDiffuse = gl_LightSource[0].diffuse * gl_FrontMaterial.diffuse * iDiffuse * shadow;	
	vec4 cSpecular = 0;
	if(underwater){
		///Add specular intensity
		cSpecular =  gl_LightSource[0].specular * gl_FrontMaterial.specular;
		cSpecular *= pow(clamp(dot(reflect(-lightVecTBN.xyz, normalTBN), viewVecTBN), 0.0, 1.0), gl_FrontMaterial.shininess )/2.0;
		cSpecular *= shadow;
	}


	return cAmbient * cBase +  cDiffuse * cBase + cSpecular;
}