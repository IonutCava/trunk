in vec4 _shadowCoord[MAX_SHADOW_CASTING_LIGHTS];

uniform sampler2DArrayShadow       texDepthMapFromLightArray;
uniform samplerCubeShadow    texDepthMapFromLightCube;
uniform sampler2DShadow      texDepthMapFromLight0;
uniform sampler2DShadow      texDepthMapFromLight1;
uniform sampler2DShadow      texDepthMapFromLight2;
uniform sampler2DShadow      texDepthMapFromLight3;
uniform sampler2D            texDiffuseProjected;

uniform float mixWeight;
uniform bool  dvd_enableShadowMapping;
////////////////////

#include "shadow_directional.frag"
#include "shadow_spot.frag"
#include "shadow_point.frag"

void projectTexture(in vec3 PoxPosInMap, inout vec4 tex){
	vec4 projectedTex = texture(texDiffuseProjected, vec2(PoxPosInMap.s, 1.0-PoxPosInMap.t));
	tex.xyz = mix(tex.xyz, projectedTex.xyz, mixWeight);
}
