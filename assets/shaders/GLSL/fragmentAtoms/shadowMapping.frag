in vec4 _shadowCoord[MAX_SHADOW_CASTING_LIGHTS];

uniform sampler2DArray       texDepthMapFromLightArray;
uniform samplerCubeShadow    texDepthMapFromLightCube;
uniform sampler2DShadow      texDepthMapFromLight[MAX_SHADOW_CASTING_LIGHTS];
uniform sampler2D            texDiffuseProjected;

uniform float mixWeight;
uniform bool  dvd_enableShadowMapping = false;

void projectTexture(in vec3 PoxPosInMap, inout vec4 tex){
	vec4 projectedTex = texture(texDiffuseProjected, vec2(PoxPosInMap.s, 1.0-PoxPosInMap.t));
	tex.xyz = mix(tex.xyz, projectedTex.xyz, mixWeight);
}

#include "shadow_directional.frag"
#include "shadow_point.frag"
#include "shadow_spot.frag"
