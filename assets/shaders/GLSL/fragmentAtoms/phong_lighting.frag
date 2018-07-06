varying vec3 vPixToLightTBN[MAX_LIGHT_COUNT];
varying vec3 vPixToLightMV[MAX_LIGHT_COUNT];
varying vec3 vPixToEyeTBN[MAX_LIGHT_COUNT];

varying vec2 _texCoord;
varying vec3 _normalMV;
varying vec4 _vertexMV;

uniform mat4  material;
uniform float opacity;

#include "texturing.frag"
#include "phong_light_loop.frag"

vec4 Phong(vec2 uv, vec3 vNormalTBN){
	// discard material if it is bellow opacity threshold
	if(opacity < 0.2) discard;

	if(hasOpacity ){
		vec4 alpha = texture(opacityMap, uv);
		if(alpha.a < 0.2) discard;
	}
	
	//Ambient color	
	vec4 cAmbient = gl_FrontLightModelProduct.sceneColor * material[0];
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
	if(tBase.a < 0.3) discard;

	float shadow = 1.0f;
	//If we have a specular map
	if(hasSpecular){//use the specular map instead
		phong_loop(normalize(vNormalTBN), texture(specularMap,uv), cDiffuse, cAmbient, cSpecular, shadow);
	}else{//Use material specular value
		phong_loop(normalize(vNormalTBN), material[2], cDiffuse, cAmbient, cSpecular, shadow);
	}

	//Add all values togheter to compute the final fragment color
	return vec4(shadow,shadow,shadow,1.0) * (cAmbient * tBase + (cDiffuse * tBase + cSpecular));	
}
