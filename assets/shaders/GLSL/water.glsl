-- Vertex
#include "vboInputData.vert"
out vec3 _pixToLight;		
out vec3 _pixToEye;	

uniform vec3 water_bb_min;
uniform vec3 water_bb_diff;
uniform mat4 dvd_lightProjectionMatrices[MAX_SHADOW_CASTING_LIGHTS];

void main(void)
{
	computeData();
	
	vec3 vertex = dvd_Vertex.xyz;

	vec3 vPositionNormalized = (vertex - water_bb_min) / water_bb_diff;
	_texCoord = vPositionNormalized.xz;
	
	_pixToLight = -(gl_LightSource[0].position.xyz);
	_pixToEye = -vec3(dvd_ModelViewMatrix * dvd_Vertex);	

	gl_Position = dvd_ModelViewProjectionMatrix * dvd_Vertex;
	
}

-- Fragment

uniform vec2 _noiseTile;
uniform vec2 _noiseFactor;
uniform float _waterShininess;
uniform float _transparencyBias;
uniform bool  underwater;
uniform sampler2D texWaterReflection;
uniform sampler2D texWaterNoiseNM;

//built-in uniforms
uniform mat4  material;
uniform float dvd_time;
uniform mat4  dvd_ModelViewMatrix;
uniform mat3  dvd_NormalMatrix;
uniform ivec2 screenDimension;

in vec3 _pixToLight;
in vec3 _pixToEye;
in vec2 _texCoord;
out vec4 _colorOut;

#include "fog.frag"
#include "shadowMapping.frag"

float Fresnel(in vec3 incident, in vec3 normal, in float bias, in float power);

const float shadow_distance_max = 200.0;
void main (void)
{
	float time2 = dvd_time * 0.00001;
	vec2 noiseUV = _texCoord * _noiseTile.x;
	vec2 uvNormal0 = noiseUV + time2;
	vec2 uvNormal1 = noiseUV;
	uvNormal1.t = uvNormal0.t;
	uvNormal1.s -= time2;
		
	vec3 normal0 = texture(texWaterNoiseNM, uvNormal0).rgb * 2.0 - 1.0;
	vec3 normal1 = texture(texWaterNoiseNM, uvNormal1).rgb * 2.0 - 1.0;
	vec3 normal = normalize(normal0+normal1);
	
	vec2 uvReflection = vec2(gl_FragCoord.x/screenDimension.x, gl_FragCoord.y/screenDimension.y);
	vec2 uvFinal = _noiseFactor * normal.rg + uvReflection;

	
	vec3 N = normalize(dvd_NormalMatrix * normal);
	vec3 L = normalize(_pixToLight);
	vec3 V = normalize(_pixToEye);
	float iSpecular = pow(clamp(dot(normalize(-reflect(L, N)), V), 0.0, 1.0), _waterShininess);

	//vec4 cAmbient = gl_LightSource[0].ambient * material[0];
	// add Diffuse
	_colorOut.rgb = texture(texWaterReflection, uvFinal).rgb;
	// add Specular
	_colorOut.rgb += vec4(gl_LightSource[0].specular * material[2] * iSpecular).rgb;

	// shadow mapping
	
	if(length(_pixToEye) < shadow_distance_max) { 
		float shadow = 1.0;
		applyShadowDirectional(max(dot(L, N), 0.0), 0, shadow);
		// add Shadow
		_colorOut.rgb *= (0.2 + 0.8 * shadow);
	}
	// add Fog
	applyFog(_colorOut);
	// calculate Transparency
	_colorOut.a	  = Fresnel(V, N, _transparencyBias, 2.0); //< Fresnel alpha

}

float Fresnel(in vec3 incident, in vec3 normal, in float bias, in float power){
	float scale = 1.0 - bias;
	return bias + pow(1.0 - dot(incident, normal), power) * scale;
}
