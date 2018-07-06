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
uniform float radius = 1.5;
uniform float power = 2.0;
uniform mat4 projectionMatrix;
uniform mat4 invProjectionMatrix;

uniform vec2 noiseScale;

// Input screen texture
layout(binding = TEXTURE_UNIT0) uniform sampler2D texNoise;
layout(binding = TEXTURE_DEPTH_MAP) uniform sampler2D texDepth;
layout(binding = TEXTURE_NORMALMAP) uniform sampler2D texNormal;

out float _ssaoOut;

float ssao(in mat3 kernelBasis, in vec3 originPos) {
    float occlusion = 0.0;

    for (int i = 0; i < KERNEL_SIZE; ++i) {
        // get sample position:
        vec3 samplePos = kernelBasis * sampleKernel[i];
        samplePos = samplePos * radius + originPos;

        // project sample position:
        vec4 offset = projectionMatrix * vec4(samplePos, 1.0);
        offset.xyz /= offset.w;
        // get sample depth:
        float sampleDepth = 2.0 * textureLod(texDepth, offset.xy * 0.5 + 0.5, 0).r - 1.0;

        float delta = offset.z - sampleDepth;

        if (delta > 0.0001 && delta < 0.005) {
            float rangeCheck = smoothstep(1.0, 0.0, radius / abs(originPos.z - sampleDepth));
            occlusion += rangeCheck * step(samplePos.z, sampleDepth);
        }
        
    }

    occlusion = 1.0 - (occlusion / float(KERNEL_SIZE));
    return pow(occlusion, power);
}

void main(void) {
    //	get view space origin:
    float originDepth = textureLod(texDepth, VAR._texCoord, 0).r;
    vec4 originPos = positionFromDepth(originDepth, invProjectionMatrix, VAR._texCoord);

    //	get view space normal:
    vec3 normal = 2.0 * normalize(texture(texNormal, VAR._texCoord).rgb) - 1.0;

    //	construct kernel basis matrix:
    vec3 rvec = normalize(2.0 * texture(texNoise, noiseScale * VAR._texCoord).rgb - 1.0);
    vec3 tangent = normalize(rvec - normal * dot(rvec, normal));
    vec3 bitangent = cross(tangent, normal);
    mat3 kernelBasis = mat3(tangent, bitangent, normal);
    
    _ssaoOut = ssao(kernelBasis, originPos.xyz);
}

--Fragment.SSAOBlur

layout(binding = TEXTURE_UNIT0) uniform sampler2D texSSAO;

uniform vec2 ssaoTexelSize;

out float _colorOut;

void main() {
    float colorOut = 0.0;
    vec2 hlim = vec2(float(-BLUR_SIZE) * 0.5 + 0.5);
    for (int x = 0; x < BLUR_SIZE; ++x) {
        for (int y = 0; y < BLUR_SIZE; ++y) {
            vec2 offset = vec2(float(x), float(y));
            offset += hlim;
            offset *= ssaoTexelSize;

            colorOut += texture(texSSAO, VAR._texCoord + offset).r;
        }
    }

    _colorOut = colorOut / float(BLUR_SIZE * BLUR_SIZE);
}

--Fragment.SSAOApply

layout(binding = TEXTURE_UNIT0) uniform sampler2D texScreen;
layout(binding = TEXTURE_UNIT1) uniform sampler2D texSSAO;

out vec4 _colorOut;

void main() {
    _colorOut.rgb = texture(texScreen, VAR._texCoord).rgb * 1.0;
                    //texture(texSSAO, VAR._texCoord).r;
}