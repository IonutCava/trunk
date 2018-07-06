varying vec4 shadowCoord[MAX_SHADOW_CASTING_LIGHTS];
uniform bool enable_shadow_mapping;

uniform sampler2DArrayShadow texDepthMapFromLightArray;
uniform samplerCubeShadow    texDepthMapFromLightCube;
uniform sampler2DShadow texDepthMapFromLight0;
uniform sampler2DShadow texDepthMapFromLight1;
uniform sampler2DShadow texDepthMapFromLight2;
uniform sampler2DShadow texDepthMapFromLight3;
uniform sampler2D texDiffuseProjected;
uniform float    mixWeight;
////////////////////

#include "shadow_directional.frag"
#include "shadow_spot.frag"
#include "shadow_point.frag"

void projectTexture(in vec3 PoxPosInMap, inout vec4 texture){
	vec4 projectedTex = texture(texDiffuseProjected, vec2(PoxPosInMap.s, 1.0-PoxPosInMap.t));
	texture.xyz = mix(texture.xyz, projectedTex.xyz, mixWeight);
}
