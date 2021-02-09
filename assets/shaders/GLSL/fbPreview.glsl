-- Fragment

#include "utility.frag"

out vec4 _colourOut;

uniform float lodLevel;
uniform bool unpack2Channel;
uniform bool unpack1Channel;
uniform uint startChannel;
uniform float multiplier;

layout(binding = TEXTURE_UNIT0) uniform sampler2D texDiffuse0;

void main()
{
    _colourOut = textureLod(texDiffuse0, VAR._texCoord, lodLevel);

    if (unpack2Channel) {
        const vec2 val = (startChannel == 2 ? _colourOut.ba : _colourOut.rg);
        _colourOut.rgb = unpackNormal(val);
    } else if (unpack1Channel) {
        _colourOut.rgb = vec3(_colourOut[startChannel]);
    } else {
        if (startChannel == 2) {
            _colourOut.rg = _colourOut.ba;
            _colourOut.b = 0.0;
        }
    }
    _colourOut.rgb *= multiplier;
    _colourOut.a = 1.0;
}

-- Fragment.LinearDepth

#include "utility.frag"

out vec4 _colourOut;

uniform vec2 zPlanes;
uniform float lodLevel;

layout(binding = TEXTURE_UNIT0) uniform sampler2D texDiffuse0;

void main()
{
    const float linearDepth = ToLinearDepth(textureLod(texDiffuse0, VAR._texCoord, lodLevel).r, zPlanes);
    /// Map back to [0 ... 1] range
    _colourOut = vec4(vec3(linearDepth / zPlanes.y), 1.0);
}

-- Fragment.Layered

out vec4 _colourOut;

layout(binding = TEXTURE_UNIT0) uniform sampler2DArray texDiffuse0;

uniform float lodLevel;
uniform int layer;

void main()
{
    _colourOut = textureLod(texDiffuse0, vec3(VAR._texCoord, layer), lodLevel);
    _colourOut.a = 1.0;
}

--Fragment.Layered.LinearDepth

#include "utility.frag"

out vec4 _colourOut;

layout(binding = TEXTURE_UNIT0) uniform sampler2DArray texDiffuse0;

uniform int layer;
uniform float lodLevel;

void main()
{
    float linearDepth = textureLod(texDiffuse0, vec3(VAR._texCoord, layer), lodLevel).r;
    _colourOut = vec4(vec3(linearDepth), 1.0);
}

--Fragment.Cube

#include "utility.frag"

out vec4 _colourOut;

layout(binding = TEXTURE_UNIT0) uniform samplerCubeArray texDiffuse0;

uniform int layer;
uniform int face;

void main() {
    vec2 uv_cube = 2.0f * VAR._texCoord - 1.0f;
    vec3 vertex = vec3(0);
    switch (face) {
    case 0:
        vertex.xyz = vec3(1.0, uv_cube.y, uv_cube.x);
        break;
    case 1:
        vertex.xyz = vec3(-1.0, uv_cube.y, -uv_cube.x);
        break;
    case 2:
        vertex.xyz = vec3(uv_cube.x, 1.0, uv_cube.y);
        break;
    case 3:
        vertex.xyz = vec3(uv_cube.x, -1.0, -uv_cube.y);
        break;
    case 4:
        vertex.xyz = vec3(-uv_cube.x, uv_cube.y, 1.0);
        break;
    case 5:
        vertex.xyz = vec3(uv_cube.x, uv_cube.y, -1.0);
        break;
    };

    _colourOut = texture(texDiffuse0, vec4(vertex, layer));
}

--Fragment.Cube.Shadow

#include "utility.frag"

out vec4 _colourOut;

layout(binding = TEXTURE_UNIT0) uniform samplerCubeArray texDiffuse0;

uniform vec2 zPlanes;
uniform int layer;
uniform int face;

void main()
{
    vec2 uv_cube = 2.0f * VAR._texCoord - 1.0f;
    vec3 vertex = vec3(0);
    switch (face) {
        case 0:
            vertex.xyz = vec3(1.0, uv_cube.y, uv_cube.x);
            break;
        case 1:
            vertex.xyz = vec3(-1.0, uv_cube.y, -uv_cube.x);
            break;
        case 2:
            vertex.xyz = vec3(uv_cube.x, 1.0, uv_cube.y);
            break;
        case 3:
            vertex.xyz = vec3(uv_cube.x, -1.0, -uv_cube.y);
            break;
        case 4:
            vertex.xyz = vec3(-uv_cube.x, uv_cube.y, 1.0);
            break;
        case 5:
            vertex.xyz = vec3(uv_cube.x, uv_cube.y, -1.0);
            break;
    };

    float depth = texture(texDiffuse0, vec4(vertex, layer)).r;
    _colourOut = vec4(vec3(depth), 1.0);
}