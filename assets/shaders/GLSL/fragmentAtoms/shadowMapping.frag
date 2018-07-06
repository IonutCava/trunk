uniform sampler2DArray       texDepthMapFromLightArray;
uniform samplerCubeShadow    texDepthMapFromLightCube;
uniform sampler2DShadow      texDepthMapFromLight[MAX_SHADOW_CASTING_LIGHTS];
uniform sampler2D            texDiffuseProjected;

uniform float mixWeight;

// set this to whatever (current cascade, current depth comparison result, anything)
int dvd_shadowMappingTempInt = -1;

void projectTexture(in vec3 PoxPosInMap, inout vec4 tex){
	vec4 projectedTex = texture(texDiffuseProjected, vec2(PoxPosInMap.s, 1.0-PoxPosInMap.t));
	tex.xyz = mix(tex.xyz, projectedTex.xyz, mixWeight);
}

#include "shadow_directional.frag"
#include "shadow_point.frag"
#include "shadow_spot.frag"
