#ifndef _UTILITY_FRAG_
#define _UTILITY_FRAG_

// Maps the depth buffer value "depthIn" to a linear [0, 1] range using dvd_zPlanes
float ToLinearDepth(in float depthIn);
// Maps the depth buffer value "depthIn" to a linear [0, 1] range using depthRange
float ToLinearDepth(in float depthIn, in vec2 depthRange);

// Maps the depth buffer value "depthIn" to a linear [0, 1] range using dvd_zPlanes and adjusts the result to make it more visible for debug purposes
float ToLinearDepthPreview(in float depthIn);
// Maps the depth buffer value "depthIn" to a linear [0, 1] range using depthRange and adjusts the result to make it more visible for debug purposes
float ToLinearDepthPreview(in float depthIn, in vec2 depthRange);

vec3 rayDirFromUV(in vec2 uv) {
    const float x = sin(M_PI * uv.y);
    const float f = 2.f * M_PI * (0.5f - uv.x);

    return vec3(x * sin(f), cos(M_PI * uv.y), x * cos(f));
}

vec2 UVDromRayDir(in vec3 dir) {
    const float y = acos(dir.y) / M_PI;

    dir = normalize(vec3(dir.x, 0.f, dir.z));

    float x = acos(dir.z) / (2.f * M_PI);

    if (x < 0.f) x  = 1.f  - x;
                 x  = 0.5f - x;
    if (x < 0.f) x += 1.f;

    return vec2(x, y);
}

#define overlay(X, Y) (X < 0.5f) ? (2.f * X * Y) : (1.f - 2.f * (1.f - X) * (1.f - Y))

#if defined(PROJECTED_TEXTURE)

uniform float projectedTextureMixWeight = 0.f;

void projectTexture(in vec3 PoxPosInMap, inout vec4 targetTexture){
    targetTexture.xyz = mix(targetTexture.rgb, 
                            texture(texProjected, vec2(PoxPosInMap.s, 1.0 - PoxPosInMap.t)).rgb,
                            projectedTextureMixWeight);
}
#endif

vec2 scaledTextureCoords(in vec2 texCoord, in vec2 scaleFactor) {
    return vec2(texCoord.s * scaleFactor.s,
                texCoord.t * scaleFactor.t);
}

vec2 scaledTextureCoords(in vec2 texCoord, in float scaleFactor) {
    return texCoord * scaleFactor;
}

vec2 unscaledTextureCoords(in vec2 texCoord, in vec2 scaleFactor) {
    return scaledTextureCoords(texCoord, 1.0f / scaleFactor);
}

vec2 unscaledTextureCoords(in vec2 texCoord, in float scaleFactor) {
    return scaledTextureCoords(texCoord, 1.0f / scaleFactor);
}

vec3 scaledTextureCoords(in vec3 texCoord, in vec3 scaleFactor) {
    return vec3(texCoord.x * scaleFactor.x,
                texCoord.y * scaleFactor.y,
                texCoord.z * scaleFactor.z);
}

vec3 scaledTextureCoords(in vec3 texCoord, in float scaleFactor) {
    return texCoord * scaleFactor;
}

vec3 unscaledTextureCoords(in vec3 texCoord, in vec3 scaleFactor) {
    return scaledTextureCoords(texCoord, 1.0f / scaleFactor);
}

vec3 unscaledTextureCoords(in vec3 texCoord, in float scaleFactor) {
    return scaledTextureCoords(texCoord, 1.0f / scaleFactor);
}

vec3 getTriPlanarBlend(in vec3 normalW) {
    // in normalW is the world-space normal of the fragment
    const vec3 blending = normalize(max(abs(normalW), 0.00001f)); // Force weights to sum to 1.0
    return blending / (blending.x + blending.y + blending.z);
}

float ToLinearDepth(in float D, in vec2 Z) { return (2 * Z.x * Z.y) / (Z.y + Z.x - (2.f * D - 1.f) * (Z.y - Z.x)); }
float ToLinearDepth(in float D)            { return ToLinearDepth(D, dvd_zPlanes); }

float ToLinearDepthPreview(in float D, in vec2 Z) { return (2 * Z.x) / (Z.y + Z.x - D * (Z.y - Z.x)); }
float ToLinearDepthPreview(in float D)            { return ToLinearDepthPreview(D, dvd_zPlanes); }

float ToLinearDepth(in float D, in mat4 projMatrix) { return projMatrix[3][2] / (D - projMatrix[2][2]); }

float ViewSpaceZ(in float D, in mat4 invProjection) { return -1.0f / (invProjection[2][3] * (2.0f * D - 1.0f) + invProjection[3][3]); }

vec3 ViewSpacePos(in vec2 texCoords, in float D, in mat4 invProjection) {
    const vec3 clipSpacePos = 2.f * vec3(texCoords, D) - vec3(1.0f);

    vec4 viewPos = vec4(vec2(invProjection[0][0], invProjection[1][1]) * clipSpacePos.xy,
                        -1.0f,
                        invProjection[2][3] * clipSpacePos.z + invProjection[3][3]);

    return(viewPos.xyz / viewPos.w);
}


#define _kLum vec3(0.299f, 0.587f, 0.114f)

#define InRangeExclusive(V, MIN, MAX) (VS > MIN && V < MAX)
#define LinStep(LOW, HIGH, V) saturate((V - LOW) / (HIGH - LOW))
#define Luminance(RGB) max(dot(RGB, _kLum), 0.0001f)

float maxComponent(in vec2 v) { return max(v.x, v.y); }
float maxComponent(in vec3 v) { return max(max(v.x, v.y), v.z); }
float maxComponent(in vec4 v) { return max(max(max(v.x, v.y), v.z), v.w); }

// ----------------- LINEAR <-> SRGB -------------------------
// Accurate variants from Frostbite notes: https://seblagarde.files.wordpress.com/2015/07/course_notes_moving_frostbite_to_pbr_v32.pdf

#define detail__gamma 2.2f
#define detail__gamma__inv  1.0f / 2.2f

#define _ToLinear(SRGB) pow(SRGB, vec3(detail__gamma))
#define _ToSRGB(RGB) pow(RGB, vec3(detail__gamma__inv))

vec3 ToLinear(in vec3 sRGBCol) { return _ToLinear(sRGBCol); }
vec4 ToLinear(in vec4 sRGBCol) { return vec4(_ToLinear(sRGBCol.rgb), sRGBCol.a); }

vec3 ToLinearAccurate(in vec3 sRGBCol) {
    const vec3 linearRGBLo = sRGBCol / 12.92f; 
    const vec3 linearRGBHi = pow((sRGBCol + 0.055f) / 1.055f, vec3(2.4f));
    
    return vec3((sRGBCol.r <= 0.04045f) ? linearRGBLo.r : linearRGBHi.r,
                (sRGBCol.g <= 0.04045f) ? linearRGBLo.g : linearRGBHi.g,
                (sRGBCol.b <= 0.04045f) ? linearRGBLo.b : linearRGBHi.b);
}

vec4 ToLinearAccurate(in vec4 sRGBCol) { return vec4(ToLinearAccurate(sRGBCol.rgb), sRGBCol.a); }

vec3 ToSRGB(vec3 linearCol) { return _ToSRGB(linearCol); }
vec4 ToSRGB(vec4 linearCol) { return vec4(_ToSRGB(linearCol.rgb), linearCol.a); }

vec3 ToSRGBAccurate(vec3 linearCol) {
    const vec3 sRGBLo = linearCol * 12.92f;
    const vec3 sRGBHi = (pow(abs(linearCol), vec3(1.0f / 2.4f)) * 1.055f) - 0.055f;

    return vec3((linearCol.r <= 0.0031308f) ? sRGBLo.r : sRGBHi.r,
                (linearCol.g <= 0.0031308f) ? sRGBLo.g : sRGBHi.g,
                (linearCol.b <= 0.0031308f) ? sRGBLo.b : sRGBHi.b);
}

vec4  ToSRGBAccurate(vec4 linearCol) { return vec4(ToSRGBAccurate(linearCol.rgb), linearCol.a); }

// ---------------------------------------------------------------------

#define dvd_screenPositionNormalised (gl_FragCoord.xy / dvd_ViewPort.zw)

float computeDepth(in vec4 posWV) {
    const float near = gl_DepthRange.near;
    const float far = gl_DepthRange.far;

    const vec4 clip_space_pos = dvd_ProjectionMatrix * posWV;

    const float ndc_depth = clip_space_pos.z / clip_space_pos.w;

    return (((far - near) * ndc_depth) + near + far) * 0.5f;
}

vec3 homogenize(in vec4 v) { return vec3((1.0f / v.w) * v); }

vec3 viewPositionFromDepth(in float depth,
                           in mat4 invProjectionMatrix,
                           in vec2 uv) {
    //to clip space
    const float x = 2.f * uv.x  - 1.f;
    const float y = 2.f * uv.y  - 1.f;
    const float z = 2.f * depth - 1.f;
    const vec4 ndc = vec4(x, y, z, 1.f);

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

vec3 turboColormap(in float x) {
    // show clipping
    if (x < 0.f)
        return vec3(0.f);
    else if (x > 1.f)
        return vec3(1.f);

    const vec4 kRedVec4 = vec4(0.13572138f, 4.61539260f, -42.66032258f, 132.13108234f);
    const vec4 kGreenVec4 = vec4(0.09140261f, 2.19418839f, 4.84296658f, -14.18503333f);
    const vec4 kBlueVec4 = vec4(0.10667330f, 12.64194608f, -60.58204836f, 110.36276771f);
    const vec2 kRedVec2 = vec2(-152.94239396f, 59.28637943f);
    const vec2 kGreenVec2 = vec2(4.27729857f, 2.82956604f);
    const vec2 kBlueVec2 = vec2(-89.90310912f, 27.34824973f);

    x = saturate(x);
    const vec4 v4 = vec4(1.0, x, x * x, x * x * x);
    const vec2 v2 = v4.zw * v4.z;
    return vec3(
        dot(v4, kRedVec4)   + dot(v2, kRedVec2),
        dot(v4, kGreenVec4) + dot(v2, kGreenVec2),
        dot(v4, kBlueVec4)  + dot(v2, kBlueVec2)
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