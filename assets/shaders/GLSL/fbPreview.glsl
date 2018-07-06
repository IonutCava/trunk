-- Vertex

void main(void)
{
    vec2 uv = vec2(0,0);
    if((gl_VertexID & 1) != 0) {
        uv.x = 1;
    }

    if((gl_VertexID & 2) != 0) {
        uv.y = 1;
    }

    VAR._texCoord = uv * 2;
    gl_Position.xy = uv * 4 - 1;
    gl_Position.zw = vec2(0,1);
}

-- Fragment

#include "utility.frag"

out vec4 _colorOut;
uniform float lodLevel = 0;
uniform bool linearSpace = false;

layout(binding = TEXTURE_UNIT0) uniform sampler2D texDiffuse0;

void main()
{
    _colorOut = textureLod(texDiffuse0, VAR._texCoord, lodLevel);
    if (!linearSpace) {
        _colorOut = ToSRGB(_colorOut);
    }
    _colorOut.a = 1.0;
}

-- Fragment.LinearDepth

#include "utility.frag"

out vec4 _colorOut;
uniform float lodLevel = 0;

layout(binding = TEXTURE_UNIT0) uniform sampler2D texDiffuse0;

void main()
{
    float linearDepth = ToLinearDepth(textureLod(texDiffuse0, VAR._texCoord, lodLevel).r);
    _colorOut = vec4(vec3(linearDepth), 1.0);
}

-- Fragment.Layered

out vec4 _colorOut;
uniform float lodLevel = 0;

layout(binding = TEXTURE_UNIT0) uniform sampler2DArray texDiffuse0;
uniform int layer;

void main()
{
    _colorOut = textureLod(texDiffuse0, vec3(VAR._texCoord, layer), lodLevel);
}

-- Fragment.Layered.LinearDepth

#include "utility.frag"

out vec4 _colorOut;


layout(binding = TEXTURE_UNIT0) uniform sampler2DArray texDiffuse0;
uniform uint layer;
uniform float lodLevel = 0;

void main()
{
    float linearDepth = ToLinearDepth(textureLod(texDiffuse0, vec3(VAR._texCoord, layer), lodLevel).r);
    _colorOut = vec4(vec3(linearDepth), 1.0);
}

--Fragment.Layered.LinearDepth.ESM

#include "utility.frag"

out vec4 _colorOut;

layout(binding = TEXTURE_UNIT0) uniform sampler2DArray texDiffuse0;
uniform uint layer;
uniform float lodLevel = 0.0;

void main()
{
    float depth = textureLod(texDiffuse0, vec3(VAR._texCoord, layer), lodLevel).r;
    //depth = 1.0 - (log(depth) / DEPTH_EXP_WARP);
	float linearDepth = ToLinearDepth(depth);
    _colorOut = vec4(vec3(linearDepth), 1.0);
}

--Fragment.Cube.LinearDepth

#include "utility.frag"

out vec4 _colorOut;

layout(binding = TEXTURE_UNIT0) uniform samplerCubeArrayShadow texDiffuse0;
uniform uint layer;
uniform uint face;
uniform float lodLevel = 0;

void main()
{

    float depth = textureLod(texDiffuse0, vec4(VAR._texCoord, face, layer), 1.0, lodLevel).r;
    //depth = 1.0 - (log(depth) / DEPTH_EXP_WARP);
    float linearDepth = ToLinearDepth(depth);
    _colorOut = vec4(vec3(linearDepth), 1.0);
}

--Fragment.Single.LinearDepth

#include "utility.frag"

out vec4 _colorOut;

layout(binding = TEXTURE_UNIT0) uniform sampler2DArrayShadow texDiffuse0;
uniform uint layer;
uniform float lodLevel = 0;

void main()
{
    float depth = textureLod(texDiffuse0, vec4(VAR._texCoord, layer, 1.0), lodLevel).r;
    float linearDepth = ToLinearDepth(depth);
    _colorOut = vec4(vec3(linearDepth), 1.0);
}