#ifndef _UTILITY_FRAG_
#define _UTILITY_FRAG_

float ToLinearDepth(in float depthIn);
float ToLinearDepth(in float depthIn, in vec2 depthRange);

vec3 ray_dir_from_uv(vec2 uv) {
    const float x = sin(M_PI * uv.y);
    const float f = 2.0f * M_PI * (0.5f - uv.x);

    return vec3(x * sin(f), cos(M_PI * uv.y), x * cos(f));
}

vec2 uv_from_ray_dir(vec3 dir) {
    vec2 uv;

    uv.y = acos(dir.y) / M_PI;

    dir.y = 0.0f;
    dir = normalize(dir);
    uv.x = acos(dir.z) / (2.0f * M_PI);
    if (dir.x < 0.0f) {
        uv.x = 1.0f - uv.x;
    }
    uv.x = 0.5f - uv.x;
    if (uv.x < 0.0f) {
        uv.x += 1.0f;
    }

    return uv;
}

float overlay(float x, float y)
{
    if (x < 0.5)
        return 2.0*x*y;
    else
        return 1.0 - 2.0*(1.0 - x)*(1.0 - y);
}

#if defined(PROJECTED_TEXTURE)

uniform float projectedTextureMixWeight;

void projectTexture(in vec3 PoxPosInMap, inout vec4 targetTexture){
    vec4 projectedTex = texture(texProjected, vec2(PoxPosInMap.s, 1.0-PoxPosInMap.t));
    targetTexture.xyz = mix(targetTexture.xyz, projectedTex.xyz, projectedTextureMixWeight);
}
#endif

bool isSamplerSet(sampler2D sampler) {
    return textureSize(sampler, 0).x > 0;
}

vec2 scaledTextureCoords(in vec2 texCoord, in vec2 scaleFactor) {
    vec2 uv = texCoord;
    uv.s *= scaleFactor.s;
    uv.t *= scaleFactor.t;
    return uv;
}

vec2 scaledTextureCoords(in vec2 texCoord, in float scaleFactor) {
    return scaledTextureCoords(texCoord, vec2(scaleFactor));
}

vec2 unscaledTextureCoords(in vec2 texCoord, in vec2 scaleFactor) {
    return scaledTextureCoords(texCoord, 1.0f / scaleFactor);
}

vec2 unscaledTextureCoords(in vec2 texCoord, in float scaleFactor) {
    return scaledTextureCoords(texCoord, 1.0f / scaleFactor);
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

float ViewSpaceZ(in float depth, in mat4 invProjection) {
    return -1.0f / (invProjection[2][3] * (2.0f * depth - 1.0f) + invProjection[3][3]);
}

vec3 ViewSpacePos(in vec2 texCoords, in float depth, in mat4 invProjection) {
    vec3 clipSpacePos = vec3(texCoords, depth) * 2.0f - vec3(1.0f);

    vec4 viewPos = vec4(vec2(invProjection[0][0], invProjection[1][1]) * clipSpacePos.xy,
                        -1.0f,
                        invProjection[2][3] * clipSpacePos.z + invProjection[3][3]);

    return(viewPos.xyz / viewPos.w);
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

// ----------------- LINEAR <-> SRGB -------------------------
// Accurate variants from Frostbite notes: https://seblagarde.files.wordpress.com/2015/07/course_notes_moving_frostbite_to_pbr_v32.pdf

#define detail__gamma 2.2f

vec3 ToLinear(in vec3 sRGBCol)  {
    return pow(sRGBCol, vec3(detail__gamma));
}

vec4 ToLinear(in vec4 sRGBCol)  {
    return vec4(ToLinear(sRGBCol.rgb), sRGBCol.a);
}

vec3 ToLinearAccurate(in vec3 sRGBCol) {
    vec3 linearRGBLo = sRGBCol / 12.92f; 
    vec3 linearRGBHi = pow((sRGBCol + 0.055f) / 1.055f, vec3(2.4f));
    
    return vec3((sRGBCol.r <= 0.04045f) ? linearRGBLo.r : linearRGBHi.r,
                (sRGBCol.g <= 0.04045f) ? linearRGBLo.g : linearRGBHi.g,
                (sRGBCol.b <= 0.04045f) ? linearRGBLo.b : linearRGBHi.b);
}

vec4 ToLinearAccurate(in vec4 sRGBCol) {
    return vec4(ToLinearAccurate(sRGBCol.rgb), sRGBCol.a);
}

vec3  ToSRGB(vec3 linearCol)  {
    return pow(linearCol, vec3(1.0f / detail__gamma));
}

vec4  ToSRGB(vec4 linearCol)  {
    return vec4(ToSRGB(linearCol.rgb), linearCol.a);
}

vec3  ToSRGBAccurate(vec3 linearCol) {
    vec3  sRGBLo = linearCol * 12.92f;
    vec3  sRGBHi = (pow(abs(linearCol), vec3(1.0f / 2.4f)) * 1.055f) - 0.055f;

    return vec3((linearCol.r <= 0.0031308f) ? sRGBLo.r : sRGBHi.r,
                (linearCol.g <= 0.0031308f) ? sRGBLo.g : sRGBHi.g,
                (linearCol.b <= 0.0031308f) ? sRGBLo.b : sRGBHi.b);
}

vec4  ToSRGBAccurate(vec4 linearCol) {
    return vec4(ToSRGBAccurate(linearCol.rgb), linearCol.a);
}
// ---------------------------------------------------------------------

#define dvd_screenPositionNormalised (gl_FragCoord.xy / dvd_ViewPort.zw)

float computeDepth(in vec4 posWV) {
    const float near = gl_DepthRange.near;
    const float far = gl_DepthRange.far;

    const vec4 clip_space_pos = dvd_ProjectionMatrix * posWV;

    const float ndc_depth = clip_space_pos.z / clip_space_pos.w;

    return (((far - near) * ndc_depth) + near + far) * 0.5f;
}

vec3 homogenize(vec4 v) { return vec3((1.0f / v.w) * v); }

vec3 viewPositionFromDepth(in float depth,
                           in mat4 invProjectionMatrix,
                           in vec2 uv) {
    //to clip space
    float x = 2.0f * uv.x  - 1.0f;
    float y = 2.0f * uv.y  - 1.0f;
    float z = 2.0f * depth - 1.0f;
    vec4 ndc = vec4(x, y, z, 1.0f);

    // to view space
    return homogenize(invProjectionMatrix * ndc);
}


#ifndef COLORMAP_SH_HEADER_GUARD
#define COLORMAP_SH_HEADER_GUARD

// Copyright 2019 Google LLC.
// SPDX-License-Identifier: Apache-2.0

// Polynomial approximation in GLSL for the Turbo colormap
// Original LUT: https://gist.github.com/mikhailov-work/ee72ba4191942acecc03fe6da94fc73f

// Authors:
//   Colormap Design: Anton Mikhailov (mikhailov@google.com)
//   GLSL Approximation: Ruofei Du (ruofei@google.com)

// See also: https://ai.googleblog.com/2019/08/turbo-improved-rainbow-colormap-for.html

vec3 turboColormap(float x) {
    // show clipping
    if (x < 0.0)
        return vec3(0.0);
    else if (x > 1.0)
        return vec3(1.0);

    const vec4 kRedVec4 = vec4(0.13572138, 4.61539260, -42.66032258, 132.13108234);
    const vec4 kGreenVec4 = vec4(0.09140261, 2.19418839, 4.84296658, -14.18503333);
    const vec4 kBlueVec4 = vec4(0.10667330, 12.64194608, -60.58204836, 110.36276771);
    const vec2 kRedVec2 = vec2(-152.94239396, 59.28637943);
    const vec2 kGreenVec2 = vec2(4.27729857, 2.82956604);
    const vec2 kBlueVec2 = vec2(-89.90310912, 27.34824973);

    x = saturate(x);
    vec4 v4 = vec4(1.0, x, x * x, x * x * x);
    vec2 v2 = v4.zw * v4.z;
    return vec3(
        dot(v4, kRedVec4) + dot(v2, kRedVec2),
        dot(v4, kGreenVec4) + dot(v2, kGreenVec2),
        dot(v4, kBlueVec4) + dot(v2, kBlueVec2)
    );
}

#endif // COLORMAP_SH_HEADER_GUARD


//ref: https://aras-p.info/texts/CompactNormalStorage.html#method08ppview
vec3 unpackNormal(in vec2 enc) {
#if 1
    const vec2 fenc = enc * 4 - 2;
    const float f = dot(fenc, fenc);
    const float g = sqrt(1 - f * 0.25f);
    return vec3(fenc * g, 1 - f * 0.5f);
#else
    vec3 ret;
    ret.xy = enc * 2 - 1;
    ret.z = sqrt(1 - dot(ret.xy, ret.xy));
    return ret;
#endif
}

vec2 packNormal(in vec3 n) {
#if 1
    const float f = sqrt(8 * n.z + 8);
    return n.xy / f + 0.5f;
#else
    return n.xy * 0.5f + 0.5f;
#endif
}

#endif //_UTILITY_FRAG_