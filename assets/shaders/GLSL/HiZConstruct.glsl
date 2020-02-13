-- Fragment

layout(binding = TEXTURE_DEPTH_MAP) uniform sampler2D depthTex;

uniform ivec2 depthInfo;
out float _depthOut;

void main() {
    
    int depthLoD = depthInfo.x;

    ivec2 lodSize = textureSize(depthTex, depthLoD);
    float depth = 0;

    if (depthInfo.y == 1) {
        ivec2 offsets[] = ivec2[](ivec2(0, 0),
                                  ivec2(0, 1),
                                  ivec2(1, 1),
                                  ivec2(1, 0));

        ivec2 coord = ivec2(gl_FragCoord.xy);
        coord *= 2;

        for (int i = 0; i < 4; i++) {
            depth = max(depth,
                        texelFetch(depthTex,
                                   clamp(coord + offsets[i], ivec2(0), lodSize - ivec2(1)),
                                   depthLoD).r);
        }
    } else {
        // need this to handle non-power of two very conservative

        vec2 offsets[] = vec2[](vec2(-1, -1),
                                vec2(0, -1),
                                vec2(1, -1),
                                vec2(-1, 0),
                                vec2(0, 0),
                                vec2(1, 0),
                                vec2(-1, 1),
                                vec2(0, 1),
                                vec2(1, 1));

        vec2 coord = VAR._texCoord;
        vec2 texel = 1.0 / (vec2(lodSize));

        for (int i = 0; i < 9; i++) {
            vec2 pos = coord + offsets[i] * texel;
            depth = max(depth,
    #if 1
                texelFetch(depthTex,
                           clamp(ivec2(pos * lodSize), ivec2(0), lodSize - ivec2(1)),
                           depthLoD).r
    #else
                textureLod(depthTex,
                           pos,
                           depthLoD).r
    #endif
            );
        }
    }

    _depthOut = depth;

    /*-----------------------------------------------------------------------
    Copyright (c) 2014, NVIDIA. All rights reserved.
    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions
    are met:
    * Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.
    * Neither the name of its contributors may be used to endorse
    or promote products derived from this software without specific
    prior written permission.
    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
    EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
    PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
    CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
    EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
    PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
    PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
    OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
    OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
    -----------------------------------------------------------------------*/
}

--Fragment.RasterGrid

//http://rastergrid.com/blog/2010/10/hierarchical-z-map-based-occlusion-culling/

#if defined(CUBE_MAP)
layout(binding = TEXTURE_DEPTH_MAP) uniform sampler2DCube LastMip;
uniform int faceIndex = 0;
#else
layout(binding = TEXTURE_DEPTH_MAP) uniform sampler2D LastMip;
#endif

out float _depthOut;
uniform ivec2 LastMipSize;

void main(void)
{
    // texture should not be linear, so this wouldn't work
    //vec4 texels = textureGather(LastMip, VAR._texCoord, 0);
    vec4 texels;
    texels.x = texture(LastMip, VAR._texCoord).x;
    texels.y = textureOffset(LastMip, VAR._texCoord, ivec2(-1,  0)).r;
    texels.z = textureOffset(LastMip, VAR._texCoord, ivec2(-1, -1)).r;
    texels.w = textureOffset(LastMip, VAR._texCoord, ivec2( 0, -1)).r;

    float maxZ = max(max(texels.x, texels.y), max(texels.z, texels.w));

    vec3 extra;
    // if we are reducing an odd-width texture then the edge fragments have to fetch additional texels
    if (((LastMipSize.x & 1) != 0) && (int(gl_FragCoord.x) == LastMipSize.x - 3)) {
        // if both edges are odd, fetch the top-left corner texel
        if (((LastMipSize.y & 1) != 0) && (int(gl_FragCoord.y) == LastMipSize.y - 3)) {
            extra.z = textureOffset(LastMip, VAR._texCoord, ivec2(1, 1)).x;
            maxZ = max(maxZ, extra.z);
        }
        extra.x = textureOffset(LastMip, VAR._texCoord, ivec2(1,  0)).x;
        extra.y = textureOffset(LastMip, VAR._texCoord, ivec2(1, -1)).x;
        maxZ = max(maxZ, max(extra.x, extra.y));
    // if we are reducing an odd-height texture then the edge fragments have to fetch additional texels
    } else if (((LastMipSize.y & 1) != 0) && (int(gl_FragCoord.y) == LastMipSize.y - 3)) {
        extra.x = textureOffset(LastMip, VAR._texCoord, ivec2( 0, 1)).x;
        extra.y = textureOffset(LastMip, VAR._texCoord, ivec2(-1, 1)).x;
        maxZ = max(maxZ, max(extra.x, extra.y));
    }

    _depthOut = maxZ;
}
