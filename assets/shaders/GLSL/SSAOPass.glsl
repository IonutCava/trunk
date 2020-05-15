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
uniform uint kernelSize = MAX_KERNEL_SIZE;

uniform float radius = 1.5f;
uniform float power = 2.0f;

uniform mat4 projectionMatrix;
uniform mat4 invProjectionMatrix;

uniform vec2 noiseScale;

// Input screen texture
layout(binding = TEXTURE_UNIT0)     uniform sampler2D texNoise;
layout(binding = TEXTURE_NORMALMAP) uniform sampler2D texNormal;
layout(binding = TEXTURE_DEPTH_MAP) uniform sampler2D texDepthMap;

out float _ssaoOut;

float ssao(in mat3 kernelBasis, in vec4 posView) {
    const float bias = 0.025f;

    float occlusion = 0.0;
    for (int i = 0; i < kernelSize; ++i) {
        // Reorient sample vector in view space ...
        const vec3 sampleVectorView = kernelBasis * sampleKernel[i];

        // ... and calculate sample point.
        const vec4 samplePointView = posView + radius * vec4(sampleVectorView, 0.0);

        // Project point and calculate NDC.
        vec4 samplePointNDC = projectionMatrix * samplePointView;

        samplePointNDC /= samplePointNDC.w;

        // Create texture coordinate out of it.
        const vec2 samplePointTexCoord = samplePointNDC.xy * 0.5 + 0.5;

        // Get sample out of depth texture
        const float zSceneNDC = viewPositionFromDepth(texture(texDepthMap, samplePointTexCoord).r, invProjectionMatrix, samplePointTexCoord).z;

        const float rangeCheck = smoothstep(0.0, 1.0, radius / abs(posView.z - zSceneNDC));

        // If scene fragment is before (smaller in z) sample point, increase occlusion.
        occlusion += (zSceneNDC >= samplePointView.z + bias ? 1.0f : 0.0f) * rangeCheck;
    }

    occlusion = 1.0f - (occlusion / (float(kernelSize)));
    return pow(occlusion, power);
}

void main(void) {
    // Calculate out of the current fragment in screen space the view space position.
    const float originDepth = texture(texDepthMap, VAR._texCoord).r;
    const vec4 originPos = viewPositionFromDepth(originDepth, invProjectionMatrix, VAR._texCoord);
    // get view space normal:
    const vec3 normal = normalize(unpackNormal(texture(texNormal, VAR._texCoord).rg));

    // construct kernel basis matrix:
    const vec3 rvec = normalize(2.0f * texture(texNoise, noiseScale * VAR._texCoord).rgb - 1.0f);
    const vec3 tangent = normalize(rvec - normal * dot(rvec, normal));
    const vec3 bitangent = cross(tangent, normal);
    const mat3 kernelBasis = mat3(tangent, bitangent, normal);
    
    _ssaoOut = ssao(kernelBasis, originPos);
}

--Fragment.SSAOBlur

layout(binding = TEXTURE_UNIT0) uniform sampler2D texSSAO;

//b - ssao
layout(location = TARGET_EXTRA) out vec4 _output;

uniform bool passThrough = false;

void main() {
#   define _colourOut (_output.b)
    if (!passThrough) {
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
    } else {
        _colourOut = 1.0f;
    }
}