uniform sampler2DArray       texDepthMapFromLightArray[MAX_SHADOW_CASTING_LIGHTS];
uniform samplerCubeShadow    texDepthMapFromLightCube[MAX_SHADOW_CASTING_LIGHTS];
uniform sampler2DShadow      texDepthMapFromLight[MAX_SHADOW_CASTING_LIGHTS];
uniform sampler2D            texDiffuseProjected;

uniform float mixWeight;

#if defined(_DEBUG)
uniform bool dvd_showShadowSplits = false;
#endif

// set this to whatever (current cascade, current depth comparison result, anything)
int _shadowTempInt = -1;

void projectTexture(in vec3 PoxPosInMap, inout vec4 tex){
	vec4 projectedTex = texture(texDiffuseProjected, vec2(PoxPosInMap.s, 1.0-PoxPosInMap.t));
	tex.xyz = mix(tex.xyz, projectedTex.xyz, mixWeight);
}

#include "shadow_directional.frag"
#include "shadow_point.frag"
#include "shadow_spot.frag"
