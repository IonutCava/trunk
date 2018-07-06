-- Vertex.Depth
uniform mat4 dvd_ModelMatrix;
uniform mat4 dvd_ModelViewProjectionMatrix;
in vec3  inVertexData;
out vec4 _vertexM;

void main(void){
	vec4 dvd_Vertex     = vec4(inVertexData,1.0);
	_vertexM = dvd_ModelMatrix * dvd_Vertex;
    gl_Position = dvd_ModelViewProjectionMatrix * dvd_Vertex;
}

-- Vertex
//based on: http://yannick.gerometta.free.fr/base.php?id=glsldemo
#include "vboInputData.vert"

uniform vec3 bbox_min;
uniform vec3 bbox_max;
out vec4 _vertexM;

#include "lightingDefaults.vert"

void main(void){
	int i = 0;
	computeData();
	vec3 vPositionNormalized = (dvd_Vertex.xyz - bbox_min.xyz) / (bbox_max.xyz - bbox_min.xyz);
	_texCoord = vPositionNormalized.xz;
	_vertexM = dvd_ModelMatrix * dvd_Vertex;
	_vertexMV = dvd_ViewMatrix * _vertexM;
	computeLightVectors();
	
	if(dvd_enableShadowMapping) {
		// Transformed position 
		//vec4 pos = dvd_ModelViewMatrix * dvd_Vertex;
		// position multiplied by the inverse of the camera matrix
		//pos = dvd_ModelViewMatrixInverse * pos;
		// position multiplied by the light matrix. The vertex's position from the light's perspective
		_shadowCoord[0] = dvd_lightProjectionMatrices[i] * dvd_Vertex;
	}

	gl_Position = dvd_ModelViewProjectionMatrix * dvd_Vertex;
}

-- Fragment.Depth

in vec4  _vertexM;
out vec4 _colorOut;

uniform float water_height;

void main (void)
{
	// Discard the fragments that are underwater when drawing in reflection

	if(_vertexM.y < water_height){
		discard;
	}

	_colorOut = vec4(1.0,1.0,1.0,1.0);
}

-- Fragment

//based on: http://yannick.gerometta.free.fr/base.php?id=glsldemo
in vec3 _lightDirection[MAX_LIGHT_COUNT];
in vec3 _viewDirection;

in vec2 _texCoord;
in vec3 _normalMV;
in vec4  _vertexM;
in vec4  _vertexMV;
out vec4 _colorOut;

uniform sampler2D texDiffuseMap;
uniform sampler2D texNormalHeightMap;
uniform sampler2D texBlend0;
uniform sampler2D texBlend1;
uniform sampler2D texBlend2;
uniform sampler2D texBlend3;
uniform sampler2D texWaterCaustics;

uniform int   dvd_lightType[MAX_LIGHT_COUNT];
uniform bool  dvd_lightEnabled[MAX_LIGHT_COUNT];
uniform float LODFactor;
uniform float detail_scale;
uniform float diffuse_scale;
uniform float water_height;
uniform float time;
uniform bool water_reflection_rendering;
uniform bool alphaTexture;
uniform vec2 zPlanes;
uniform mat4 material;
uniform vec3 bbox_min;
uniform vec4 dvd_lightAmbient;

#define LIGHT_DIRECTIONAL		0
#define LIGHT_OMNIDIRECTIONAL	1
#define LIGHT_SPOT				2

vec4 NormalMapping(vec2 uv, vec3 pixelToLightTBN, bool underwater);
vec4 CausticsColor();

///Global NDotL, basically
float iDiffuse;

#include "fog.frag"
#include "shadowMapping.frag"

bool isUnderWater(){
	return _vertexM.y < water_height;
}

void main (void)
{
	// Discard the fragments that are underwater when drawing in reflection
	bool underwater = isUnderWater();
	if(water_reflection_rendering && underwater){
		discard;
	}
	
	vec4 color = NormalMapping(_texCoord, _lightDirection[0], underwater);
	
	if(underwater) {
		float alpha = (water_height - _vertexM.y) / (2*(water_height - bbox_min.y));
		color = (1-alpha) * color + alpha * CausticsColor();
	}
    applyFog(color);

	_colorOut = color;
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

vec4 NormalMapping(vec2 uv, vec3 pixelToLightTBN, bool underwater)
{	
	vec3 lightVecTBN = normalize(pixelToLightTBN);
	vec3 viewVecTBN = normalize(_viewDirection);
    //float z = 1.0 - (zDepth) / zPlanes.y;
	vec2 uv_detail = uv * detail_scale;
	vec2 uv_diffuse = uv * diffuse_scale;
	
	vec3 normalTBN = texture(texNormalHeightMap, uv_detail).rgb * 2.0 - 1.0;
	normalTBN = normalize(normalTBN);
	iDiffuse = max(dot(lightVecTBN.xyz, normalTBN), 0.0);	// diffuse intensity. NDotL

	vec4 tBase[4];

	tBase[0] = texture(texBlend0, uv_diffuse);
	tBase[1] = texture(texBlend1, uv_diffuse);	
	tBase[2] = texture(texBlend2, uv_diffuse);
	tBase[3] = texture(texBlend3, uv_diffuse);
	
	vec4 DiffuseMap = texture(texDiffuseMap, uv);
	
	vec4 cBase;

	if(_vertexM.y < water_height)
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
	float distance = length(_viewDirection);

	if(distance < distance_max) {
		applyShadowDirectional(iDiffuse, 0, shadow);
		shadow = 1.0 - (1.0-shadow) * (distance_max-distance) / distance_max;
	}

	vec4 cAmbient = dvd_lightAmbient * material[0];
	vec4 cDiffuse = material[1] * iDiffuse * shadow;

    if(dvd_lightEnabled[0]){
        
	    cAmbient += gl_LightSource[0].ambient * material[0];
	    cDiffuse *= gl_LightSource[0].diffuse;
    }

	vec4 cSpecular = material[2];
	if(underwater){
		///Add specular intensity
        
        if(dvd_lightEnabled[0]){
            cSpecular =  gl_LightSource[0].specular * material[2];
			cSpecular *= pow(clamp(dot(reflect(-lightVecTBN.xyz, normalTBN), viewVecTBN), 0.0, 1.0), material[3].x )/2.0;
        }
		cSpecular *= shadow;
	}

	//return cAmbient * cBase +  cDiffuse * cBase + cSpecular;
	return cAmbient * cBase +  cDiffuse * cBase;
}