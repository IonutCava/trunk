#define LIGHT_DIRECTIONAL		0
#define LIGHT_OMNIDIRECTIONAL	1
#define LIGHT_SPOT				2

uniform int lightType[MAX_LIGHT_COUNT];
uniform int light_count;

///Global NDotL, basically
float iDiffuse;

#include "phong_point_light.frag"
#include "phong_spot_light.frag"
#include "phong_directional_light.frag"
#include "shadowMapping.frag"



void applyLight(in int light, in vec3 normal,in vec4 specularIn, inout vec4 diffuse, inout vec4 ambient, inout vec4 specular, inout float shadow){
	//Normalized Light/Normal/Eye vectors in TBN space
	vec3 L = normalize(vPixToLightTBN[light]);
	vec3 V = normalize(vPixToEyeTBN[light]);
	
	if(lightType[light] == LIGHT_DIRECTIONAL){
		phong_directionalLight(light,specularIn,L,normal,V,ambient,diffuse,specular);
		if(light < MAX_SHADOW_CASTING_LIGHTS){
			applyShadowDirectional(light, shadow); 
		}
		return;
	}
	
	if(lightType[light] == LIGHT_OMNIDIRECTIONAL){
		phong_pointLight(light,specularIn,L,normal,V,ambient,diffuse,specular);
		if(light < MAX_SHADOW_CASTING_LIGHTS){
			applyShadowPoint(light, shadow); 
		}
		return;
	}

	if(lightType[light] == LIGHT_SPOT){
		phong_spotLight(light,specularIn,L,normal,V,ambient,diffuse,specular);
		if(light < MAX_SHADOW_CASTING_LIGHTS){
			applyShadowSpot(light, shadow); 
		}
		return;
	}
}

void phong_loop(in vec3 normal, in vec4 specularIn, inout vec4 diffuse, inout vec4 ambient, inout vec4 specular, inout float shadow){
	if(light_count == 0) return;
	applyLight(0, normal,specularIn,diffuse,ambient,specular,shadow);
#if MAX_LIGHT_COUNT >= 2
	if(light_count == 1) return;
	applyLight(1, normal,specularIn,diffuse,ambient,specular,shadow);
#endif
#if MAX_LIGHT_COUNT >= 3
	if(light_count == 2) return;
	applyLight(2, normal,specularIn,diffuse,ambient,specular,shadow);
#endif
#if MAX_LIGHT_COUNT >= 4
	if(light_count == 3) return;
	applyLight(3, normal,specularIn,diffuse,ambient,specular,shadow);
#endif
#if MAX_LIGHT_COUNT >= 5
	if(light_count == 4) return;
	applyLight(4, normal,specularIn,diffuse,ambient,specular,shadow);
#endif
#if MAX_LIGHT_COUNT >= 6
	if(light_count == 5) return;
	applyLight(5, normal,specularIn,diffuse,ambient,specular,shadow);
#endif
#if MAX_LIGHT_COUNT >= 7
	if(light_count == 6) return;
	applyLight(6, normal,specularIn,diffuse,ambient,specular,shadow);
#endif
#if MAX_LIGHT_COUNT == 8
	if(light_count == 7) return;
	applyLight(7, normal,specularIn,diffuse,ambient,specular,shadow);
#endif
#if MAX_LIGHT_COUNT > 8
	///Apply the rest of the lights
	for(int i = 8; i < MAX_LIGHT_COUNT; i++){
		if(light_count == i) return;
		applyLight(i, normal,specularIn,diffuse,ambient,specular,shadow);
	}
#endif
}