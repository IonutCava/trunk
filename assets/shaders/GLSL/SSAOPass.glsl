-- Vertex

void main(void)
{
    vec2 uv = vec2(0,0);
    if((gl_VertexID & 1) != 0)uv.x = 1;
    if((gl_VertexID & 2) != 0)uv.y = 1;

    VAR._texCoord = uv * 2;
    gl_Position.xy = uv * 4 - 1;
    gl_Position.zw = vec2(0,1);
}

-- Fragment.SSAOCalc

/*******************************************************************************
Copyright (C) 2013 John Chapman

This software is distributed freely under the terms of the MIT License.
See "license.txt" or "http://copyfree.org/licenses/mit/license.txt".
*******************************************************************************/
/*And: https://github.com/McNopper/OpenGL/blob/master/Example28/shader/ssao.frag.glsl */

#include "utility.frag"

uniform vec3 sampleKernel[KERNEL_SIZE];
uniform float radius = 5.5;
uniform float power = 2.0;
uniform mat4 projectionMatrix;
uniform mat4 invProjectionMatrix;

// Input screen texture
layout(binding = TEXTURE_UNIT0) uniform sampler2D texNoise;
layout(binding = TEXTURE_DEPTH_MAP) uniform sampler2D texDepth;
layout(binding = TEXTURE_NORMALMAP) uniform sampler2D texNormal;

layout(location = 0) out vec3 _ssaoOut;

// This constant removes artifacts caused by neighbour fragments with minimal depth difference.
#define CAP_MIN_DISTANCE 0.0001

// This constant avoids the influence of fragments, which are too far away.
#define CAP_MAX_DISTANCE 0.005

float ssao(in mat3 kernelBasis, in vec3 originPos) {
    float occlusion = 0.0;

    for (int i = 0; i < KERNEL_SIZE; ++i) {
        // get sample position:
        vec3 samplePos = kernelBasis * sampleKernel[i];
        samplePos = samplePos * radius + originPos;

        // project sample position:
        vec4 offset = projectionMatrix * vec4(samplePos, 1.0);
        offset.xyz /= offset.w;
        vec2 coords = offset.xy * 0.5 + 0.5;

        // get sample depth:
        float sampleDepth = 2.0 * texture(texDepth, coords).r - 1.0;

        float delta = offset.z - sampleDepth;

        // range check & accumulate:
        /*float rangeCheck = smoothstep(0.0, 1.0, radius / abs(originPos.z - sampleDepth));
        occlusion += rangeCheck * step(sampleDepth, samplePos.z);*/
        if (delta > CAP_MIN_DISTANCE && delta < CAP_MAX_DISTANCE)
        {
            occlusion += 1.0;
        }
    }

    //occlusion = 1.0 - (occlusion / float(KERNEL_SIZE));
    occlusion = 1.0 - occlusion / (float(KERNEL_SIZE) - 1.0);
    return pow(occlusion, power);
    //return  1.0 - occlusion / (float(KERNEL_SIZE) - 1.0);
}

void main(void) {
    //	get noise texture coords:
    vec2 noiseTexCoords = vec2(textureSize(texDepth, 0)) /
                          vec2(textureSize(texNoise, 0));
    noiseTexCoords *= VAR._texCoord;

    //	get view space origin:
    float originDepth = texture(texDepth, VAR._texCoord).r;
    vec4 originPos = positionFromDepth(originDepth, invProjectionMatrix, VAR._texCoord);

    //	get view space normal:
    vec3 normal = 2.0 * normalize(texture(texNormal, VAR._texCoord).rgb) - 1.0;

    //	construct kernel basis matrix:
    vec3 rvec = 2.0 * texture(texNoise, noiseTexCoords).rgb - 1.0;
    vec3 tangent = normalize(rvec - normal * dot(rvec, normal));
    vec3 bitangent = cross(tangent, normal);
    mat3 kernelBasis = mat3(tangent, bitangent, normal);
    
    _ssaoOut = vec3(ssao(kernelBasis, originPos.xyz));
}

--Fragment.SSAOApply

layout(binding = TEXTURE_UNIT0) uniform sampler2D texScreen;
layout(binding = TEXTURE_UNIT1) uniform sampler2D texSSAO;

out vec4 _colorOut;

void main() {
    float ssaoFilter = texture(texSSAO, VAR._texCoord).r;
    _colorOut = texture(texScreen, VAR._texCoord);

    if (ssaoFilter > 0) {
        _colorOut.rgb = _colorOut.rgb * ssaoFilter;
    }
}

--Fragment.SSAOBlur

layout(binding = TEXTURE_UNIT0) uniform sampler2D texScreen;
layout(binding = TEXTURE_UNIT1) uniform sampler2D texSSAO;

layout(location = 0) out vec3 _colorOut;
layout(location = 1) out vec4  _screenOut;

void main() {
    vec2 texelSize = 1.0 / vec2(textureSize(texSSAO, 0));

    _colorOut = vec3(0.0);
    vec2 hlim = vec2(float(-BLUR_SIZE) * 0.5 + 0.5);
    for (int x = 0; x < BLUR_SIZE; ++x) {
        for (int y = 0; y < BLUR_SIZE; ++y) {
            vec2 offset = vec2(float(x), float(y));
            offset += hlim;
            offset *= texelSize;

            _colorOut += texture(texSSAO, VAR._texCoord + offset).rgb;
        }
    }

    _colorOut = _colorOut / float(BLUR_SIZE * BLUR_SIZE);
    _screenOut = texture(texScreen, VAR._texCoord);
}