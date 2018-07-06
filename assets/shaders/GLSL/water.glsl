-- Vertex
#include "vboInputData.vert"
out vec3 vPixToLight;		
out vec3 vPixToEye;	
out vec4 _vertexMV;
out vec4 vVertexFromLightView;

uniform vec3 water_bb_min;
uniform vec3 water_bb_max;
uniform mat4 dvd_lightProjectionMatrices[MAX_SHADOW_CASTING_LIGHTS];

void main(void)
{
	computeData();
	

	_vertexMV = dvd_Vertex;
	vec3 vPositionNormalized = (dvd_Vertex.xyz - water_bb_min.xyz) / (water_bb_max.xyz - water_bb_min.xyz);
	_texCoord = vPositionNormalized.xz;
	
	vPixToLight = -(gl_LightSource[0].position.xyz);
	vPixToEye = -vec3(dvd_ModelViewMatrix * dvd_Vertex);	

	vVertexFromLightView = dvd_lightProjectionMatrices[0] * dvd_Vertex;
	gl_Position = dvd_ModelViewProjectionMatrix * dvd_Vertex;
	
}

-- Fragment

uniform ivec2 screenDimension;
uniform float noise_tile;
uniform float noise_factor;
uniform float time;
uniform float water_shininess;
uniform bool  underwater;
uniform mat4  material;
uniform mat4  dvd_ModelViewMatrix;
uniform sampler2D texWaterReflection;
uniform sampler2D texWaterNoiseNM;

in vec3 vPixToLight;
in vec3 vPixToEye;
in vec4 _vertexMV;
in vec4 vVertexFromLightView;
in vec2 _texCoord;
out vec4 _colorOut;
///Global NDotL, basically
float iDiffuse;

#include "fog.frag"
#include "shadowMapping.frag"

float Fresnel(vec3 incident, vec3 normal, float bias, float power);

void main (void)
{
	float time2 = time * 0.00001;
	vec2 uvNormal0 = _texCoord*noise_tile;
	uvNormal0.s += time2;
	uvNormal0.t += time2;
	vec2 uvNormal1 = _texCoord*noise_tile;
	uvNormal1.s -= time2;
	uvNormal1.t += time2;
		
	vec3 normal0 = texture(texWaterNoiseNM, uvNormal0).rgb * 2.0 - 1.0;
	vec3 normal1 = texture(texWaterNoiseNM, uvNormal1).rgb * 2.0 - 1.0;
	vec3 normal = normalize(normal0+normal1);
	
	vec2 uvReflection = vec2(gl_FragCoord.x/screenDimension.x, gl_FragCoord.y/screenDimension.y);
	vec2 uvFinal = uvReflection.xy + noise_factor*normal.xy;
	if(!underwater){
		uvFinal.x = 1.0 - uvFinal.x;
	}
	vec4 cDiffuse = texture(texWaterReflection, uvFinal);
	
	vec3 N = normalize(vec3(dvd_ModelViewMatrix * vec4(normal.x, normal.z, normal.y, 0.0)));
	vec3 L = normalize(vPixToLight);
	vec3 V = normalize(vPixToEye);
	float iSpecular = pow(clamp(dot(reflect(-L, N), V), 0.0, 1.0), water_shininess);
	iDiffuse = max(dot(L, N), 0.0);
	//vec4 cAmbient = gl_LightSource[0].ambient * material[0];
	vec4 cSpecular = gl_LightSource[0].specular * material[2] * iSpecular;
	/////////////////////////
	// SHADOW MAPS
	float distance_max = 200.0;
	float shadow = 1.0;
	float distance = length(vPixToEye);
	if(distance < distance_max) {
		applyShadowDirectional(iDiffuse, 0, shadow);
	}
	/////////////////////////

	vec4 color = (0.2 + 0.8 * shadow) * cDiffuse + shadow * cSpecular;
	// FRESNEL ALPHA
	color.a	= Fresnel(V, N, 0.5, 2.0);
    applyFog(color);

	_colorOut = color;
}


float Fresnel(vec3 incident, vec3 normal, float bias, float power){
	float scale = 1.0 - bias;
	return bias + pow(1.0 - dot(incident, normal), power) * scale;
}
