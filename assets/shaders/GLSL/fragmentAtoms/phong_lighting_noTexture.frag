
in vec2 _texCoord;
in vec3 _normalMV;
in vec4 _vertexMV;

uniform mat4  material;
uniform float opacity;

#include "phong_light_loop.frag"

vec4 Phong(vec3 vNormalTBN){
	// discard material if it is bellow opacity threshold
	if(opacity < ALPHA_DISCARD_THRESHOLD) discard;
	
	//Ambient color	
	vec4 cAmbient = dvd_lightAmbient * material[0];
	//Diffuse color
	vec4 cDiffuse;
	//Specular color
	vec4 cSpecular;
	float shadow = 1.0;
	//Use material specular value
	phong_loop(normalize(vNormalTBN), material[2], cDiffuse, cAmbient, cSpecular, shadow);
	
	//Add all values togheter to compute the final fragment color
	return (cAmbient + cDiffuse + cSpecular) * (0.2 + 0.8 * shadow);	
}
