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
vec2  saturate(vec2 v)  { return clamp(v, 0.0, 1.0); }
vec3  saturate(vec3 v)  { return clamp(v, 0.0, 1.0); }
vec4  saturate(vec4 v)  { return clamp(v, 0.0, 1.0); }

float overlay(float x, float y)
{
    if (x < 0.5)
        return 2.0*x*y;
    else
        return 1.0 - 2.0*(1.0 - x)*(1.0 - y);
}

void projectTexture(in vec3 PoxPosInMap, inout vec4 targetTexture){
    vec4 projectedTex = texture(texDiffuseProjected, vec2(PoxPosInMap.s, 1.0-PoxPosInMap.t));
    targetTexture.xyz = mix(targetTexture.xyz, projectedTex.xyz, projectedTextureMixWeight);
}

bool isSamplerSet(sampler2D sampler) {
    return textureSize(sampler, 0).x > 0;
}

vec2 scaledTextureCoords(in vec2 texCoord, in vec2 scaleFactor) {
    return vec2(.5, .5) + ((texCoord - vec2(.5, .5)) * scaleFactor);
}

vec2 scaledTextureCoords(in vec2 texCoord, in float scaleFactor) {
    return scaledTextureCoords(texCoord, vec2(scaleFactor));
}

//Box Projected Cube Environment Mapping by Bartosz Czuba
vec3 bpcem(in vec3 v, vec3 Emax, vec3 Emin, vec3 Epos)
{
    vec3 pos = vec3(1.0);
    //e.g.: vec3 rVec = bpcem(reflect(vVec,nVec),EnvBoxMax,EnvBoxMin,EnvBoxPos); //bpcem-izing reflection coordinates
    //      vec3 env = textureCube(envMap, rVec).rgb;
    vec3 nrdir = normalize(v);
    vec3 rbmax = (Emax - pos) / nrdir;
    vec3 rbmin = (Emin - pos) / nrdir;

    vec3 rbminmax;
    rbminmax.x = (nrdir.x>0.0) ? rbmax.x : rbmin.x;
    rbminmax.y = (nrdir.y>0.0) ? rbmax.y : rbmin.y;
    rbminmax.z = (nrdir.z>0.0) ? rbmax.z : rbmin.z;
    float fa = min(min(rbminmax.x, rbminmax.y), rbminmax.z);
    vec3 posonbox = pos + nrdir * fa;
    return posonbox - Epos;
}

vec3 applyFogColour(in vec3 colour){
    const float LOG2 = 1.442695;
    float zDepth = gl_FragCoord.z / gl_FragCoord.w;
    return mix(dvd_fogColour, colour, clamp(exp2(-dvd_fogDensity * dvd_fogDensity * zDepth * zDepth * LOG2), 0.0, 1.0));
}

vec4 applyFog(in vec4 colour) { 
    return vec4(mix(applyFogColour(colour.rgb), colour.rgb, dvd_fogDensity), colour.a);
}

float ToLinearDepth(in float depthIn) {
#if defined(USE_SCENE_ZPLANES)
    float n = dvd_ZPlanesCombined.z;
    float f = dvd_ZPlanesCombined.w * 0.5;
#else
    float n = dvd_ZPlanesCombined.x;
    float f = dvd_ZPlanesCombined.y * 0.5;
#endif

    return (2 * n) / (f + n - (depthIn) * (f - n));
}

float ToLinearDepth(in float depthIn, in mat4 projMatrix) {
    return projMatrix[3][2] / (depthIn - projMatrix[2][2]);
}

bool InRangeExclusive(in float value, in float min, in float max) {
    return value > min && value < max;
}

float luminance(in vec3 rgb) {
    const vec3 kLum = vec3(0.299, 0.587, 0.114);
    return max(dot(rgb, kLum), 0.0001); // prevent zero result
}

const float gamma = 2.2;
const float invGamma = 1.0 / gamma;
const vec3 invGammaVec = vec3(invGamma);
const vec3 gammaVec = vec3(gamma);

float ToLinear(float v) { return pow(v, gamma); }
vec3  ToLinear(vec3 v)  { return pow(v, gammaVec); }
vec4  ToLinear(vec4 v)  { return vec4(pow(v.rgb, gammaVec), v.a); }

float ToSRGB(float v) { return pow(v, invGamma); }
vec3  ToSRGB(vec3 v)  { return pow(v, invGammaVec); }
vec4  ToSRGB(vec4 v)  { return vec4(pow(v.rgb, invGammaVec), v.a);}


vec3 unpackNormal(vec2 packedNormal)
{
    vec3 normal;
    normal.xy = packedNormal.xy * (255.0 / 128.0) - 1.0;
    normal.z = sqrt(1 - normal.x*normal.x - normal.y * normal.y);
    return normal;
}

vec2 packNormal(vec3 normal)
{
    return vec2(normal.xy * 0.5 + 0.5);
}

float Gloss(vec3 bump)
{
    float gloss = 1.0;

    /*if (do_toksvig)
    {
        // Compute the "Toksvig Factor"
        float rlen = 1.0 / saturate(length(bump));
        gloss = 1.0 / (1.0 + power*(rlen - 1.0));
    }

    if (do_toksmap)
    {
        // Fetch pre-computed "Toksvig Factor"
        // and adjust for specified power
        gloss = texture2D(gloss_map, texcoord).r;
        gloss /= mix(power / baked_power, 1.0, gloss);
    }
    */
    return gloss;
}

vec2 getScreenPositionNormalised() {
    return gl_FragCoord.xy * dvd_invScreenDimensions();
}

#endif //_LIGHTING_DEFAULTS_FRAG_