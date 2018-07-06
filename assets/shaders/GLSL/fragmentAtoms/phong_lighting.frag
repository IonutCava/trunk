varying vec4 vPixToLightTBN[1];
varying vec3 vPixToEyeTBN;
varying vec3 vVertexMV;
varying vec3 vPixToLightMV;
varying vec3 vLightDirMV;
varying vec4 texCoord[2];
varying vec3 vNormalMV;

//Material properties
//vec4 ambient = material[0];
//vec4 diffuse = material[1];
//vec4 specular = material[2];
//float shininess = material[3].x;
//vec4 emmissive = vec4(material[3].yzw,1.0f);
uniform mat4  material;
uniform float opacity;

#define LIGHT_DIRECTIONAL		0.0
#define LIGHT_OMNIDIRECTIONAL	1.0
#define LIGHT_SPOT				2.0

#include "texturing.frag"
#include "shadowMapping.frag"

//true -> use opacity map
uniform bool hasOpacity;
//true -> use specular map 
uniform bool hasSpecular;

//Opacity and specular maps
uniform sampler2D opacityMap;
uniform sampler2D specularMap;
uniform int mode;
#define MODE_SHADOW 4

vec4 Phong(vec2 uv, vec3 vNormalTBN, vec3 vEyeTBN, vec4 vLightTBN){
	if(opacity < 0.2) discard;
	//if(mode == MODE_SHADOW) return vec4(0,0,0,0);
	float att = 1.0;
	//If the light isn't directional, compute attenuation
	if(vLightTBN.w != LIGHT_DIRECTIONAL) {
		float dist = length(vLightTBN.xyz);
		att = 1.0/(gl_LightSource[0].constantAttenuation + gl_LightSource[0].linearAttenuation*dist + gl_LightSource[0].quadraticAttenuation*dist*dist);
		att = max(att, 0.0);	
	}
	//Normalized Light/Normal/Eye vectors in TBN space
	vec3 L = normalize(vLightTBN.xyz);
	vec3 N = normalize(vNormalTBN.xyz);
	vec3 V = normalize(vEyeTBN.xyz);
	
	//vec4 ambient = material[0];
	//vec4 diffuse = material[1];
	//vec4 specular = material[2];
	//float shininess = material[3].x;
	//vec4 emmissive = vec4(material[3].yzw,1.0f);
	
	//Use material specular value
	vec4 specular = material[2];
	//If we have a specular map
	if(hasSpecular){
		//use the specular map instead
		specular = texture2D(specularMap,uv);
	}
	//Diffuse intensity
	float iDiffuse = max(dot(L, N), 0.0);
	//Specular intensity based on material shininess
	float iSpecular = pow(clamp(dot(reflect(-L, N), V), 0.0, 1.0), material[3].x );
	//Ambient color	
	vec4 cAmbient = gl_LightSource[0].ambient * material[0];
	//Diffuse color
	vec4 cDiffuse = gl_LightSource[0].diffuse * material[1] * iDiffuse;	
	//Specular color
	vec4 cSpecular = gl_LightSource[0].specular * specular * iSpecular;	
	//Adding diffuse texture(s)
	vec4 tBase[2];
	//set the texture color vector to 1 so we don't change final color if no texture
	tBase[0]  = vec4(1.0,1.0,1.0,1.0);
	//If we have at least 1 texture
	if(textureCount > 0){
		//Get the texture color
		//Get the texture color. use Replace for the first texture
		applyTexture2D(texDiffuse0, texDiffuse0Op, 0, uv, tBase[0]);
		//If we have a second diffuse texture
		if(textureCount > 1){
			//Apply the second texture over the first
			applyTexture2D(texDiffuse1, texDiffuse1Op, 0, uv, tBase[0]);
		} 
		//If the texture's alpha channel is less than 1/3, discard current fragment
		if(tBase[0].a < 0.3) discard;
	}
	float shadow = 1.0;
	//If we have a spot light
	if(vLightTBN.w > 1.5){
		//Handle the spot's halo
		if(dot(normalize(vPixToLightMV.xyz), normalize(-vLightDirMV.xyz)) < gl_LightSource[0].spotCosCutoff){
			cDiffuse = vec4(0.0, 0.0, 0.0, 1.0);
			cSpecular = vec4(0.0, 0.0, 0.0, 1.0);
		}else{
			if(enable_shadow_mapping != 0) {
				/////////////////////////
				// SHADOW MAPS
				vec3 vPixPosInDepthMap;
				//Compute shadow value for current fragment
				shadow = ShadowMapping(vPixPosInDepthMap);
				//And add shadow value to current diffuse color and specular values
				cDiffuse = (shadow) * cDiffuse;
				cSpecular = (shadow) * cSpecular;
				cAmbient = (shadow) * cAmbient;
				// Texture projection :
				if(enable_shadow_mapping == 2) {
					vec4 cProjected = texture2D(texDiffuseProjected, vec2(vPixPosInDepthMap.s, 1.0-vPixPosInDepthMap.t));
					tBase[0].xyz = mix(tBase[0].xyz, cProjected.xyz, shadow/2.0);
				}
			}
		}
	//If it's not a spot light
	}else{
		//ToDo
	}
	cAmbient = cAmbient + gl_FrontLightModelProduct.sceneColor * material[0];
	//Add all values togheter to compute the final fragment color
	vec4 color = cAmbient * tBase[0] + (cDiffuse * tBase[0] + cSpecular) * att;

	return color;	
}
