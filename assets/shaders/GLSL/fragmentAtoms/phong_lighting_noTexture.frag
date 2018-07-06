varying vec3 vPixToLightTBN[MAX_LIGHT_COUNT];
varying vec3 vPixToLightMV[MAX_LIGHT_COUNT];
varying vec3 vPixToEyeTBN[MAX_LIGHT_COUNT];

varying vec2 _texCoord;
varying vec3 _normalMV;
varying vec4 _vertexMV;

uniform mat4  material;
uniform float opacity;

#include "phong_light_loop.frag"

vec4 Phong(vec3 vNormalTBN){
	// discard material if it is bellow opacity threshold
	if(opacity < 0.2) discard;
	
	//Ambient color	
	vec4 cAmbient = gl_LightModel.ambient * material[0];
	//Diffuse color
	vec4 cDiffuse;
	//Specular color
	vec4 cSpecular;
	float shadow = 1.0f;
	//Use material specular value
	phong_loop(normalize(vNormalTBN), material[2], cDiffuse, cAmbient, cSpecular, shadow);
	
	//Add all values togheter to compute the final fragment color
	return vec4(shadow,shadow,shadow,1.0) * (cAmbient + (cDiffuse + cSpecular));	
}
