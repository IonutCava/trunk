#ifndef _LIGHTING_DEFAULTS_FRAG_
#define _LIGHTING_DEFAULTS_FRAG_

#include "nodeBufferedInput.cmn"

uniform float projectedTextureMixWeight;
layout(binding = TEXTURE_PROJECTION) uniform sampler2D texDiffuseProjected;

#define PRECISION 0.000001

float DIST_TO_ZERO(float val) {
    return 1.0 - (step(-PRECISION, val) * (1.0 - step(PRECISION, val)));
}

float saturate(float v) { return clamp(v, 0.0, 1.0); }
vec3  saturate(vec3 v)  { return clamp(v, 0.0, 1.0); }
vec4  saturate(vec4 v)  { return clamp(v, 0.0, 1.0); }

vec3 pow_vec3(vec3 v, float p) { return vec3(pow(v.x, p), pow(v.y, p), pow(v.z, p)); }

void projectTexture(in vec3 PoxPosInMap, inout vec4 targetTexture){
    vec4 projectedTex = texture(texDiffuseProjected, vec2(PoxPosInMap.s, 1.0-PoxPosInMap.t));
    targetTexture.xyz = mix(targetTexture.xyz, projectedTex.xyz, projectedTextureMixWeight);
}

vec3 applyFogColor(in vec3 color){
    const float LOG2 = 1.442695;
    float zDepth = gl_FragCoord.z / gl_FragCoord.w;
    return mix(dvd_fogColor, color, clamp(exp2(-dvd_fogDensity * dvd_fogDensity * zDepth * zDepth * LOG2), 0.0, 1.0));
}

vec4 applyFog(in vec4 color) { 
    return vec4(mix(applyFogColor(color.rgb), color.rgb, dvd_fogDensity), color.a);
}

const float gamma = 2.2;

vec3 ToLinear(vec3 v) { return pow_vec3(v,     gamma); }
vec4 ToLinear(vec4 v) { return vec4(pow_vec3(v.rgb,     gamma), v.a); }
vec3 ToSRGB(vec3 v)   { return pow_vec3(v, 1.0/gamma); }
vec4 ToSRGB(vec4 v)   { return vec4(pow_vec3(v.rgb, 1.0/gamma), v.a);}


#endif //_LIGHTING_DEFAULTS_FRAG_