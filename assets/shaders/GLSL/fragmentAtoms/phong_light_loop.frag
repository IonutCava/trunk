in vec3 _lightDirection[MAX_LIGHT_COUNT];
in float _attenuation[MAX_LIGHT_COUNT];
in vec3 _viewDirection;

#define LIGHT_DIRECTIONAL		0
#define LIGHT_OMNIDIRECTIONAL	1
#define LIGHT_SPOT				2

struct MaterialProperties {
	vec4 ambient;
	vec4 diffuse;
	vec4 specular;
	float shadowFactor;
};

uniform bool dvd_lightCastsShadows[MAX_LIGHT_COUNT];
uniform bool dvd_lightEnabled[MAX_LIGHT_COUNT];
uniform int  dvd_lightType[MAX_LIGHT_COUNT];
uniform int  dvd_lightCount;
uniform vec4 dvd_lightAmbient;

//Specular and opacity maps are available even for non-textured geometry
#if defined(USE_OPACITY_MAP)
//Opacity and specular maps
uniform sampler2D opacityMap;
#endif
#if defined(USE_SPECULAR_MAP)
uniform sampler2D specularMap;
#endif

#include "phong_point_light.frag"
#include "phong_spot_light.frag"
#include "phong_directional_light.frag"
#include "shadowMapping.frag"

void applyLight(in int light, 
				in int lightType, 
				in bool lightCastsShadows,
				in vec2 texCoord, 
				in vec3 normal, 
				in vec3 viewDirection,
				inout MaterialProperties materialProp)
{
	//Normalized Light/Normal/Eye vectors in TBN space
	vec3 L = normalize(_lightDirection[light]);
	vec3 R = normalize(-reflect(L,normal));
	//Specular intensity based on material shininess
	float iSpecular = clamp(pow(max(dot(R, viewDirection), 0.0), material[3].x ), 0.0, 1.0);
	float NdotL = max(dot(normal, L), 0.0);

#if defined(USE_SPECULAR_MAP)
	vec4 specularValue = texture(specularMap, texCoord)
#else
	vec4 specularValue = material[2];
#endif

	if(lightType == LIGHT_DIRECTIONAL){
		phong_directionalLight(light, iSpecular, NdotL, specularValue, materialProp);

		if(light >= MAX_SHADOW_CASTING_LIGHTS) return;
		if(lightCastsShadows) applyShadowDirectional(NdotL, light, materialProp.shadowFactor); 

	}else if(lightType == LIGHT_OMNIDIRECTIONAL){
		phong_pointLight(light, iSpecular, NdotL, specularValue, materialProp);

		if(light >= MAX_SHADOW_CASTING_LIGHTS) return;
		if(lightCastsShadows) applyShadowPoint(light, materialProp.shadowFactor); 

    }else{//if(lightType == LIGHT_SPOT)
		phong_spotLight(light, iSpecular, NdotL, specularValue, materialProp);

		if(light >= MAX_SHADOW_CASTING_LIGHTS) return;
		if(lightCastsShadows) applyShadowSpot(light, materialProp.shadowFactor); 
	}
}

void phong_loop(in vec2 texCoord, in vec3 normal, inout MaterialProperties materialProp){
	vec3 viewDirection = normalize(_viewDirection);
	if(dvd_lightCount == 0) return;
	if(dvd_lightEnabled[0]) applyLight(0, dvd_lightType[0], dvd_lightCastsShadows[0], texCoord, normal, viewDirection, materialProp);
#if MAX_LIGHT_COUNT >= 2
	if(dvd_lightCount == 1) return;
	if(dvd_lightEnabled[1]) applyLight(1, dvd_lightType[1], dvd_lightCastsShadows[1], texCoord, normal, viewDirection, materialProp);
#endif
#if MAX_LIGHT_COUNT >= 3
	if(dvd_lightCount == 2) return;
	if(dvd_lightEnabled[2]) applyLight(2, dvd_lightType[2], dvd_lightCastsShadows[2], texCoord, normal, viewDirection, materialProp);
#endif
#if MAX_LIGHT_COUNT >= 4
	if(dvd_lightCount == 3) return;
	if(dvd_lightEnabled[3]) applyLight(3, dvd_lightType[3], dvd_lightCastsShadows[3], texCoord, normal, viewDirection, materialProp);
#endif
#if MAX_LIGHT_COUNT >= 5
	if(dvd_lightCount == 4) return;
	if(dvd_lightEnabled[4]) applyLight(4, dvd_lightType[4], dvd_lightCastsShadows[4], texCoord, normal, viewDirection, materialProp);
#endif
#if MAX_LIGHT_COUNT >= 6
	if(dvd_lightCount == 5) return;
	if(dvd_lightEnabled[5]) applyLight(5, dvd_lightType[5], dvd_lightCastsShadows[5], texCoord, normal, viewDirection, materialProp);
#endif
#if MAX_LIGHT_COUNT >= 7
	if(dvd_lightCount == 6) return;
	if(dvd_lightEnabled[6]) applyLight(6, dvd_lightType[6], dvd_lightCastsShadows[6], texCoord, normal, viewDirection, materialProp);
#endif
#if MAX_LIGHT_COUNT == 8
	if(dvd_lightCount == 7) return;
	if(dvd_lightEnabled[7]) applyLight(7, dvd_lightType[7], dvd_lightCastsShadows[7], texCoord, normal, viewDirection, materialProp);
#endif
#if MAX_LIGHT_COUNT > 8
	///Apply the rest of the lights
	for(int i = 8; i < MAX_LIGHT_COUNT; i++){
		if(dvd_lightCount == i) return;
		if(dvd_lightEnabled[i]) applyLight(i, dvd_lightType[i], dvd_lightCastsShadows[i], texCoord, normal, viewDirection, materialProp);
	}
#endif
}