--Fragment.SSAOCalc

/*******************************************************************************
Copyright (C) 2013 John Chapman

This software is distributed freely under the terms of the MIT License.
See "license.txt" or "http://copyfree.org/licenses/mit/license.txt".
*******************************************************************************/
/*And: https://github.com/McNopper/OpenGL/blob/master/Example28/shader/ssao.frag.glsl */

#include "utility.frag"

uniform mat4 projectionMatrix;
uniform mat4 invProjectionMatrix;
uniform vec2 SSAO_NOISE_SCALE;
uniform vec2 zPlanes;
uniform float SSAO_RADIUS;
uniform float SSAO_BIAS;
uniform float SSAO_INTENSITY;
uniform float maxRange;
uniform float fadeStart;
uniform vec4 sampleKernel[SSAO_SAMPLE_COUNT];

// Input screen texture
layout(binding = TEXTURE_UNIT0)         uniform sampler2D texNoise;
#if defined(COMPUTE_HALF_RES)
layout(binding = TEXTURE_DEPTH_MAP)     uniform sampler2D texDepthMap;
#else
layout(binding = TEXTURE_SCENE_NORMALS) uniform sampler2D texNormal;
layout(binding = TEXTURE_DEPTH_MAP)     uniform sampler2D texDepthMap;
#endif

out float _ssaoOut;

#define GetDepth(UV) texture(texDepthMap, UV).r

#if defined(COMPUTE_HALF_RES)
#define GetNormal(UV) texture(texDepthMap, UV).gb
#else
#define GetNormal(UV) texture(texNormal, UV).rg
#endif

//ref1: https://github.com/McNopper/OpenGL/blob/master/Example28/shader/ssao.frag.glsl
//ref2: https://github.com/itoral/vkdf/blob/9622f6a9e6602e06c5a42507202ad5a7daf917a4/data/spirv/ssao.deferred.frag.input
void main(void) {
    // Calculate out of the current fragment in screen space the view space position.
    const float sceneDepth = GetDepth(VAR._texCoord);
    const float linDepth = ToLinearDepthPreview(sceneDepth, zPlanes);
    if (linDepth <= maxRange) {
        const vec3 posView = ViewSpacePos(VAR._texCoord, sceneDepth, invProjectionMatrix);

        // Normal gathering.
        const vec3 normalView = normalize(unpackNormal(GetNormal(VAR._texCoord)));

        // Calculate the rotation  for the kernel.
        const vec3 randomVector = texture(texNoise, VAR._texCoord * SSAO_NOISE_SCALE).xyz * 2.f - 1.f;

        // Using Gram-Schmidt process to get an orthogonal vector to the normal vector.
        // The resulting tangent is on the same plane as the random and normal vector. 
        // see http://en.wikipedia.org/wiki/Gram%E2%80%93Schmidt_process
        // Note: No division by <u,u> needed, as this is for normal vectors 1. 
        const vec3 tangentView = normalize(randomVector - dot(randomVector, normalView) * normalView);
        const vec3 bitangentView = cross(normalView, tangentView);

        // Final matrix to reorient the kernel depending on the normal and the random vector.
        const mat3 kernelMatrix = mat3(tangentView, bitangentView, normalView);

        // Go through the kernel samples and create occlusion factor.
        float occlusion = 0.0;
        for (int i = 0; i < SSAO_SAMPLE_COUNT; ++i) {
            // Reorient sample vector in view space and calculate sample point.
            const vec3 sampleVectorView = posView + (kernelMatrix * sampleKernel[i].xyz) * SSAO_RADIUS;

            // Project point and calculate NDC.
            // Convert sample XY to texture coordinate space and sample from the
            // Position texture to obtain the scene depth at that XY coordinate
            const vec2 samplePointTexCoord = 0.5f * homogenize(projectionMatrix * vec4(sampleVectorView, 1.0f)).xy + 0.5f;

            const float zSceneNDC = ViewSpaceZ(GetDepth(samplePointTexCoord), invProjectionMatrix);

            // If the depth for that XY position in the scene is larger than
            // the sample's, then the sample is occluded by scene's geometry and
            // contributes to the occlussion factor.
            const float rangeCheck = smoothstep(0.f, 1.f, SSAO_RADIUS / abs(posView.z - zSceneNDC));
            occlusion += (zSceneNDC >= sampleVectorView.z + SSAO_BIAS ? 1.f : 0.f) * rangeCheck;
        }
        // We output ambient intensity in the range [0,1]
        _ssaoOut = saturate(pow(1.f - (occlusion / SSAO_SAMPLE_COUNT), SSAO_INTENSITY) + smoothstep(fadeStart, maxRange, linDepth));
    } else {
        _ssaoOut = 1.f;
    }
    
}

--Fragment.SSAOBlur

#include "utility.frag"

//ref: https://github.com/itoral/vkdf/blob/9622f6a9e6602e06c5a42507202ad5a7daf917a4/data/spirv/ssao-blur.deferred.frag.input
layout(binding = TEXTURE_UNIT0)      uniform sampler2D texSSAO;
layout(binding = TEXTURE_DEPTH_MAP)  uniform sampler2D texDepthMap;

uniform vec2 zPlanes;
uniform vec2 texelSize;
uniform float depthThreshold;

//r - ssao
layout(location = TARGET_EXTRA) out vec2 _output;
#define _colourOut _output.r

/**
 * Does a simple blur pass over the input SSAO texture. To prevent the "halo"
 * effect at the edges of geometry caused by blurring together pixels with
 * and without occlusion, we only blur together pixels that have "similar"
 * depth values.
 */
void main() {
    const float linear_depth_ref = ToLinearDepth(texture(texDepthMap, VAR._texCoord).r, zPlanes);


    int sample_count = 1;
    float result = texture(texSSAO, VAR._texCoord).r;
#if 1
    for (int x = -BLUR_SIZE; x < BLUR_SIZE; ++x) {
        for (int y = -BLUR_SIZE; y < BLUR_SIZE; ++y) {
            if (x != 0 || y != 0) {
                const vec2 tex_offset = min(vec2(1.0), max(vec2(0.0), VAR._texCoord + vec2(x, y) * texelSize));

                const float linear_depth = ToLinearDepth(texture(texDepthMap, tex_offset).r, zPlanes);
                if (abs(linear_depth - linear_depth_ref) < depthThreshold)
                {
                    result += texture(texSSAO, tex_offset).r;
                    ++sample_count;
                }
            }
        }
    }

    _colourOut = result / float(sample_count);
#else
    _colourOut = result;
#endif
}

--Fragment.SSAOPassThrough

layout(binding = TEXTURE_UNIT0) uniform sampler2D texSSAO;

//r - ssao
layout(location = TARGET_EXTRA) out vec4 _output;

void main() {
#   define _colourOut (_output.r)
    _colourOut = texture(texSSAO, VAR._texCoord).r;
}

--Fragment.SSAODownsample

layout(binding = TEXTURE_SCENE_NORMALS) uniform sampler2D texNormal;
layout(binding = TEXTURE_DEPTH_MAP)     uniform sampler2D texDepthMap;

out vec3 _outColour;

float most_representative() {
    float samples[] = float[](
        textureOffset(texDepthMap, VAR._texCoord, ivec2(0, 0)).r,
        textureOffset(texDepthMap, VAR._texCoord, ivec2(0, 1)).r,
        textureOffset(texDepthMap, VAR._texCoord, ivec2(1, 0)).r,
        textureOffset(texDepthMap, VAR._texCoord, ivec2(1, 1)).r);

    float centr = (samples[0] + samples[1] + samples[2] + samples[3]) / 4.0f;
    float dist[] = float[](
        abs(centr - samples[0]),
        abs(centr - samples[1]),
        abs(centr - samples[2]),
        abs(centr - samples[3]));

    float max_dist = max(max(dist[0], dist[1]), max(dist[2], dist[3]));
    float rem_samples[3];
    int rejected_idx[3];

    int j = 0; int i; int k = 0;
    for (i = 0; i < 4; i++) {
        if (dist[i] < max_dist) {
            rem_samples[j] = samples[i];
            j++;
        } else {
            /* for the edge case where 2 or more samples
               have max_dist distance from the centroid */
            rejected_idx[k] = i;
            k++;
        }
    }

    /* also for the edge case where 2 or more samples
       have max_dist distance from the centroid */
    if (j < 2) {
        for (i = 3; i > j; i--) {
            rem_samples[i] = samples[rejected_idx[k]];
            k--;
        }
    }

    centr = (rem_samples[0] + rem_samples[1] + rem_samples[2]) / 3.0;

    dist[0] = abs(rem_samples[0] - centr);
    dist[1] = abs(rem_samples[1] - centr);
    dist[2] = abs(rem_samples[2] - centr);

    float min_dist = min(dist[0], min(dist[1], dist[2]));
    for (int i = 0; i < 3; i++) {
        if (dist[i] == min_dist)
            return rem_samples[i];
    }

    /* should never reach */
    return samples[0];
}


void main()
{
   const ivec2 offsets[] = ivec2[](ivec2(0, 0),
                                   ivec2(0, 1),
                                   ivec2(1, 1),
                                   ivec2(1, 0));
   float d[] = float[] (
      textureOffset(texDepthMap, VAR._texCoord, offsets[0]).r,
      textureOffset(texDepthMap, VAR._texCoord, offsets[1]).r,
      textureOffset(texDepthMap, VAR._texCoord, offsets[2]).r,
      textureOffset(texDepthMap, VAR._texCoord, offsets[3]).r);
 
   vec2 pn[] = vec2[](
       textureOffset(texNormal, VAR._texCoord, offsets[0]).rg,
       textureOffset(texNormal, VAR._texCoord, offsets[1]).rg,
       textureOffset(texNormal, VAR._texCoord, offsets[2]).rg,
       textureOffset(texNormal, VAR._texCoord, offsets[3]).rg);

   const float best_depth = most_representative();

   int i = 0;
   for (; i < 4; ++i) {
      if (abs(best_depth - d[i]) <= M_EPSILON) {
          break;
      }
   }

   _outColour = vec3(d[i], pn[i]);
}

--Fragment.SSAOUpsample

#include "utility.frag"

layout(binding = TEXTURE_UNIT0)     uniform sampler2D texSSAOLinear;
layout(binding = TEXTURE_UNIT1)     uniform sampler2D texSSAONearest;
layout(binding = TEXTURE_NORMALMAP) uniform sampler2D texSSAONormalsDepth;
layout(binding = TEXTURE_DEPTH_MAP) uniform sampler2D texDepthMap;

out float _ssaoOut;

float nearest_depth_ao() {
    float d[] = float[](
        textureOffset(texSSAONormalsDepth, VAR._texCoord, ivec2(0, 0)).r,
        textureOffset(texSSAONormalsDepth, VAR._texCoord, ivec2(0, 1)).r,
        textureOffset(texSSAONormalsDepth, VAR._texCoord, ivec2(1, 0)).r,
        textureOffset(texSSAONormalsDepth, VAR._texCoord, ivec2(1, 1)).r);

    float ao[] = float[](
        textureOffset(texSSAONearest, VAR._texCoord, ivec2(0, 0)).r,
        textureOffset(texSSAONearest, VAR._texCoord, ivec2(0, 1)).r,
        textureOffset(texSSAONearest, VAR._texCoord, ivec2(1, 0)).r,
        textureOffset(texSSAONearest, VAR._texCoord, ivec2(1, 1)).r);

    float d0 = texture(texDepthMap, VAR._texCoord).r;
    float min_dist = 1.0;

    int best_depth_idx;
    for (int i = 0; i < 4; i++) {
        float dist = abs(d0 - d[i]);

        if (min_dist > dist) {
            min_dist = dist;
            best_depth_idx = i;
        }
    }

    return ao[best_depth_idx];
}

void main() {
    vec3 n[] = vec3[](
        unpackNormal(textureOffset(texSSAONormalsDepth, VAR._texCoord, ivec2(0, 0)).gb),
        unpackNormal(textureOffset(texSSAONormalsDepth, VAR._texCoord, ivec2(0, 1)).gb),
        unpackNormal(textureOffset(texSSAONormalsDepth, VAR._texCoord, ivec2(1, 0)).gb),
        unpackNormal(textureOffset(texSSAONormalsDepth, VAR._texCoord, ivec2(1, 1)).gb));

    float dot01 = dot(n[0], n[1]);
    float dot02 = dot(n[0], n[2]);
    float dot03 = dot(n[0], n[3]);

    float min_dot = min(dot01, min(dot02, dot03));
    float s = step(0.997f, min_dot);

    _ssaoOut = mix(nearest_depth_ao(), texture(texSSAOLinear, VAR._texCoord).r, s);
}