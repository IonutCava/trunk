#ifndef _UTILITY_FRAG_
#define _UTILITY_FRAG_

#include "nodeBufferedInput.cmn"


#define PRECISION 0.000001

//ref: http://theorangeduck.com/page/avoiding-shader-conditionals
#define when_eq(x, y) (1.0f - abs(sign(x - y)))
#define when_neq(x, y) abs(sign(x - y))
#define when_gt(x, y)  max(sign(x - y), 0.0f)
#define when_lt(x, y)  max(sign(y - x), 0.0f)
#define when_ge(x, y) (1.0f - when_lt(x, y))
#define when_le(x, y) (1.0f - when_gt(x, y))
#define AND(a, b) (a * b)
#define OR(a, b) min(a + b, 1.0f)
#define XOR(a, b) ((a + b) % 2.0f)
#define NOT(a) (1.0f - a)

float DIST_TO_ZERO(float val) {
    return 1.0 - (step(-PRECISION, val) * (1.0 - step(PRECISION, val)));
}

float saturate(float v) { return clamp(v, 0.0, 1.0); }
vec2  saturate(vec2 v)  { return clamp(v, 0.0, 1.0); }
vec3  saturate(vec3 v)  { return clamp(v, 0.0, 1.0); }
vec4  saturate(vec4 v)  { return clamp(v, 0.0, 1.0); }

float maxComponent(vec2 v) { return max(v.x, v.y); }
float maxComponent(vec3 v) { return max(maxComponent(v.xy), v.z); }
float maxComponent(vec4 v) { return max(maxComponent(v.xyz), v.w); }

float ToLinearDepth(in float depthIn);
float ToLinearDepth(in float depthIn, in vec2 depthRange);

float ToLinearPreviewDepth(in float depthIn);
float ToLinearPreviewDepth(in float depthIn, in vec2 depthRange);

float overlay(float x, float y)
{
    if (x < 0.5)
        return 2.0*x*y;
    else
        return 1.0 - 2.0*(1.0 - x)*(1.0 - y);
}

#if defined(PROJECTED_TEXTURE)

uniform float projectedTextureMixWeight;

layout(binding = TEXTURE_PROJECTION) uniform sampler2D texDiffuseProjected;

void projectTexture(in vec3 PoxPosInMap, inout vec4 targetTexture){
    vec4 projectedTex = texture(texDiffuseProjected, vec2(PoxPosInMap.s, 1.0-PoxPosInMap.t));
    targetTexture.xyz = mix(targetTexture.xyz, projectedTex.xyz, projectedTextureMixWeight);
}
#endif

bool isSamplerSet(sampler2D sampler) {
    return textureSize(sampler, 0).x > 0;
}

vec2 scaledTextureCoords(in vec2 texCoord, in vec2 scaleFactor) {
    return vec2(.5f, .5f) + ((texCoord - vec2(.5f, .5f)) * scaleFactor);
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

vec3 applyFogColour(in float depth, in vec3 colour, in vec2 depthRange) {
    const float LOG2 = 1.442695;
    float zDepth = ToLinearDepth(depth, depthRange);
    return mix(dvd_fogColour, colour, clamp(exp2(-dvd_fogDensity * dvd_fogDensity * zDepth * zDepth * LOG2), 0.0, 1.0));
}

vec4 applyFog(in float depth, in vec4 colour, in vec2 depthRange) {
    return vec4(applyFogColour(depth, colour.rgb, depthRange), colour.a);
}

float ToLinearPreviewDepth(in float depthIn, in vec2 depthRange) {
    float zNear = depthRange.x;
    float zFar  = depthRange.y;
    float depthSample = depthIn;

    return (2 * zNear) / (zFar + zNear - depthSample * (zFar - zNear));
}

float ToLinearPreviewDepth(in float depthIn) {
    return ToLinearPreviewDepth(depthIn, dvd_zPlanes);
}

float ToLinearDepth(in float depthIn, in vec2 depthRange) {
    float zNear = depthRange.x;
    float zFar  = depthRange.y ;
    float depthSample = 2.0f * depthIn - 1.0f;

    return (2 * zNear * zFar) / (zFar + zNear - depthSample * (zFar - zNear));
}

float ToLinearDepth(in float depthIn) {
    return ToLinearDepth(depthIn, dvd_zPlanes);
}

float ToLinearDepth(in float depthIn, in mat4 projMatrix) {
    return projMatrix[3][2] / (depthIn - projMatrix[2][2]);
}

bool InRangeExclusive(in float value, in float min, in float max) {
    return value > min && value < max;
}

float linstep(float low, float high, float v) {
    return clamp((v - low) / (high - low), 0.0, 1.0);
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


vec3 unpackNormal(vec2 enc)
{
    vec2 fenc = enc * 4 - 2;
    float f = dot(fenc, fenc);
    float g = sqrt(1 - f * 0.25f);
    return vec3(fenc * g, 1 - f * 0.5f);
}

vec2 packNormal(vec3 n)
{
    float f = sqrt(8 * n.z + 8);
    return n.xy / f + 0.5f;
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

#if defined(NEED_DEPTH_TEXTURE)
layout(binding = TEXTURE_DEPTH_MAP) uniform sampler2D texDepthMap;

float getDepthValue(vec2 screenNormalisedPos) {
    return textureLod(texDepthMap, screenNormalisedPos, 0).r;
}
#endif


float computeDepth(in vec4 posWV) {
    float near = gl_DepthRange.near;
    float far = gl_DepthRange.far;

    vec4 clip_space_pos = dvd_ProjectionMatrix * posWV;

    float ndc_depth = clip_space_pos.z / clip_space_pos.w;

    return (((far - near) * ndc_depth) + near + far) * 0.5f;
}

vec4 positionFromDepth(in float depth,
                       in mat4 invProjectionMatrix,
                       in vec2 uv) {

    vec4 pos = vec4(2.0f * uv.x - 1.0f,
        2.0f * uv.y - 1.0f,
        2.0f * depth - 1.0f,
        1.0f);

    pos = invProjectionMatrix * pos;
    pos /= pos.w;

    return pos;
}
#endif //_UTILITY_FRAG_