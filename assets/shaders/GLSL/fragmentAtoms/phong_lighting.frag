
in vec2 _texCoord;
in vec3 _normalMV;
in vec4 _vertexMV;

uniform mat4  material;
uniform float opacity;

#include "texturing.frag"
#include "phong_light_loop.frag"

vec4 Phong(vec2 uv, vec3 vNormalTBN){
    
#if defined(USE_OPACITY_MAP)
	vec4 alphaMask = texture(opacityMap, uv);
	float alpha = alphaMask.a;
	// discard material if it is bellow opacity threshold
	if(alpha < ALPHA_DISCARD_THRESHOLD) discard;
#else
    if(opacity< ALPHA_DISCARD_THRESHOLD) discard;
#endif

	//Ambient color	
	vec4 cAmbient = dvd_lightAmbient * material[0];
	//Diffuse color
	vec4 cDiffuse;
	//Specular color
	vec4 cSpecular;

	///this shader is generated only for nodes with at least 1 texture
	vec4 tBase;
	//Get the texture color. use Replace for the first texture
	applyTexture(texDiffuse0, texDiffuse0Op, 0, uv, tBase);
	
	//If we have a second diffuse texture
	if(textureCount > 1){
		//Apply the second texture over the first
		applyTexture(texDiffuse1, texDiffuse1Op, 0, uv, tBase);
	}
	//If the texture's alpha channel is less than 1/3, discard current fragment
	if(tBase.a < ALPHA_DISCARD_THRESHOLD) discard;

	float shadow = 1.0;
	//If we have a specular map
#if defined(USE_SPECULAR_MAP)
    phong_loop(normalize(vNormalTBN), texture(specularMap,uv), cDiffuse, cAmbient, cSpecular, shadow);
#else//Use material specular value
	phong_loop(normalize(vNormalTBN), material[2], cDiffuse, cAmbient, cSpecular, shadow);
#endif

	//Add all values togheter to compute the final fragment color
	return cAmbient * tBase + vec4(shadow,shadow,shadow,1.0) * (cDiffuse * tBase + cSpecular);	
}
