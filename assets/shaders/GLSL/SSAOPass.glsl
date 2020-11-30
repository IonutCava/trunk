--Fragment.SSAOCalc

/*******************************************************************************
Copyright (C) 2013 John Chapman

This software is distributed freely under the terms of the MIT License.
See "license.txt" or "http://copyfree.org/licenses/mit/license.txt".
*******************************************************************************/
/*And: https://github.com/McNopper/OpenGL/blob/master/Example28/shader/ssao.frag.glsl */

#include "utility.frag"

// This constant removes artifacts caused by neighbour fragments with minimal depth difference.
#define CAP_MIN_DISTANCE 0.0001
// This constant avoids the influence of fragments, which are too far away.
#define CAP_MAX_DISTANCE 0.005

uniform vec3 sampleKernel[MAX_KERNEL_SIZE];

uniform vec2 SSAO_NOISE_SCALE;
uniform float SSAO_RADIUS = 1.5f;
uniform float SSAO_BIAS = 0.05f;
uniform float SSAO_INTENSITY = 2.0f;


uniform mat4 projectionMatrix;
uniform mat4 invProjectionMatrix;

// Input screen texture
layout(binding = TEXTURE_UNIT0)         uniform sampler2D texNoise;
#if defined(COMPUTE_HALF_RES)
layout(binding = TEXTURE_DEPTH_MAP)     uniform sampler2D texSSAONormalsDepth;
#else
layout(binding = TEXTURE_SCENE_NORMALS) uniform sampler2D texNormal;
layout(binding = TEXTURE_DEPTH_MAP)     uniform sampler2D texDepthMap;
#endif

out float _ssaoOut;

float getDepth(in vec2 uv) {
#if defined(COMPUTE_HALF_RES)
    return texture(texSSAONormalsDepth, uv).a;
#else
    return texture(texDepthMap, uv).r;
#endif
}

vec3 getNormals(in vec2 uv) {
#if defined(COMPUTE_HALF_RES)
    return normalize(texture(texSSAONormalsDepth, uv).rgb);
#else
    return normalize(unpackNormal(texture(texNormal, uv).rg));
#endif
}

void main(void) {
    // Retrieve eye-space position of the current fragment reconstructing it from the depth buffer.
    const vec3 position = viewPositionFromDepth(getDepth(VAR._texCoord), invProjectionMatrix, VAR._texCoord);
    // Retrieve eye-space normal of the current fragment from the GBuffer
    const vec3 normal = getNormals(VAR._texCoord);

    // Compute a rotated TBN transform based on the noise vector
    const vec3 noise = texture(texNoise, VAR._texCoord * SSAO_NOISE_SCALE).xyz;
    const vec3 tangent = normalize(noise - normal * dot(noise, normal));
    const vec3 bitangent = cross(normal, tangent);
    const mat3 TBN = mat3(tangent, bitangent, normal);
    
    /// Accumulate occlusion for each kernel sample
    float occlusion = 0.0;
    for (int i = 0; i < SSAO_SAMPLE_COUNT; ++i) {
        // Compute sample position in view space
        vec3 sample_i = TBN * sampleKernel[i];
        sample_i = position + sample_i * SSAO_RADIUS;

        // Compute sample XY coordinates in NDC space
        const vec2 sample_i_ndc = homogenize(projectionMatrix * vec4(sample_i, 1.0f)).xy;

        // Convert sample XY to texture coordinate space and sample from the
        // Position texture to obtain the scene depth at that XY coordinate
        const vec2 sample_i_uv = sample_i_ndc * 0.5f + 0.5f;
        //const float ref_depth = ViewSpaceZ(getDepth(sample_i_uv), projectionMatrix);
        const float ref_depth = viewPositionFromDepth(getDepth(sample_i_uv), invProjectionMatrix, sample_i_uv).z;

        // If the depth for that XY position in the scene is larger than
        // the sample's, then the sample is occluded by scene's geometry and
        // contributes to the occlussion factor.
        if (ref_depth >= sample_i.z + SSAO_BIAS) {
            occlusion += smoothstep(0.0, 1.0, SSAO_RADIUS / abs(position.z - ref_depth));
        }
    }

    // We output ambient intensity in the range [0,1]
    _ssaoOut = pow(1.0 - (occlusion / SSAO_SAMPLE_COUNT), SSAO_INTENSITY);
}

--Fragment.SSAOBlur

layout(binding = TEXTURE_UNIT0) uniform sampler2D texSSAO;

//r - ssao
layout(location = TARGET_EXTRA) out vec4 _output;

void main() {
#   define _colourOut (_output.r)
    vec2 texelSize = 1.0 / vec2(textureSize(texSSAO, 0));

    const int end = int(BLUR_SIZE / 2);
    const int start = int(end * -1);

    float colourOut = 0.0;
    for (int x = start; x < end; ++x) {
        for (int y = start; y < end; ++y) {
            vec2 offset = vec2(float(x), float(y)) * texelSize;
            colourOut += texture(texSSAO, VAR._texCoord + offset).r;
        }
    }

    _colourOut = colourOut / float(BLUR_SIZE * BLUR_SIZE);
   
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

#include "utility.frag"

layout(binding = TEXTURE_SCENE_NORMALS) uniform sampler2D texNormal;
layout(binding = TEXTURE_DEPTH_MAP)     uniform sampler2D texDepthMap;

out vec4 _outColour;

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
   float d[] = float[] (
      textureOffset(texDepthMap, VAR._texCoord, ivec2(0, 0)).x,
      textureOffset(texDepthMap, VAR._texCoord, ivec2(0, 1)).x,
      textureOffset(texDepthMap, VAR._texCoord, ivec2(1, 0)).x,
      textureOffset(texDepthMap, VAR._texCoord, ivec2(1, 1)).x);

   vec2 pn[] = vec2[](
       textureOffset(texNormal, VAR._texCoord, ivec2(0, 0)).rg,
       textureOffset(texNormal, VAR._texCoord, ivec2(0, 1)).rg,
       textureOffset(texNormal, VAR._texCoord, ivec2(1, 0)).rg,
       textureOffset(texNormal, VAR._texCoord, ivec2(1, 1)).rg);
 
   float best_depth = most_representative();
 
   for (int i = 0; i < 4; i++) {
      if (best_depth == d[i]) {
          _outColour = vec4(length(pn[i]) <= M_EPSILON ? vec3(0.0f) : unpackNormal(pn[i]), d[i]);
         return;
      }
   }
}

--Fragment.SSAOUpsample

layout(binding = TEXTURE_UNIT0)     uniform sampler2D texSSAOLinear;
layout(binding = TEXTURE_UNIT1)     uniform sampler2D texSSAONearest;
layout(binding = TEXTURE_NORMALMAP) uniform sampler2D texSSAONormalsDepth;
layout(binding = TEXTURE_DEPTH_MAP) uniform sampler2D texDepthMap;

out float _ssaoOut;

float nearest_depth_ao() {
    float d[] = float[](
        textureOffset(texSSAONormalsDepth, VAR._texCoord, ivec2(0, 0)).a,
        textureOffset(texSSAONormalsDepth, VAR._texCoord, ivec2(0, 1)).a,
        textureOffset(texSSAONormalsDepth, VAR._texCoord, ivec2(1, 0)).a,
        textureOffset(texSSAONormalsDepth, VAR._texCoord, ivec2(1, 1)).a);

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
        textureOffset(texSSAONormalsDepth, VAR._texCoord, ivec2(0, 0)).rgb,
        textureOffset(texSSAONormalsDepth, VAR._texCoord, ivec2(0, 1)).rgb,
        textureOffset(texSSAONormalsDepth, VAR._texCoord, ivec2(1, 0)).rgb,
        textureOffset(texSSAONormalsDepth, VAR._texCoord, ivec2(1, 1)).rgb);

    float dot01 = dot(n[0], n[1]);
    float dot02 = dot(n[0], n[2]);
    float dot03 = dot(n[0], n[3]);

    float min_dot = min(dot01, min(dot02, dot03));
    float s = step(0.997f, min_dot);

    _ssaoOut = mix(nearest_depth_ao(), texture(texSSAOLinear, VAR._texCoord).r, s);
}