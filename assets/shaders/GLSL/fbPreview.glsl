-- Fragment

#include "utility.frag"

out vec4 _colourOut;
uniform float lodLevel = 0;
uniform bool unpack2Channel = false;
uniform bool unpack1Channel = false;
uniform bool startOnBlue = false;

layout(binding = TEXTURE_UNIT0) uniform sampler2D texDiffuse0;

void main()
{
    _colourOut = textureLod(texDiffuse0, VAR._texCoord, lodLevel);

    if (unpack2Channel) {
        _colourOut.rgb = unpackNormal((startOnBlue ? _colourOut.ba : _colourOut.rg));
    } else if (unpack1Channel) {
        _colourOut.rgb = vec3((startOnBlue ? _colourOut.b : _colourOut.r));
    } else {
        if (startOnBlue) {
            _colourOut.rg = _colourOut.ba;
            _colourOut.b = 0.0;
        }
    }

    _colourOut.a = 1.0;
}

-- Fragment.LinearDepth

#include "utility.frag"

out vec4 _colourOut;
uniform float lodLevel = 0;
uniform vec2 zPlanes;

layout(binding = TEXTURE_UNIT0) uniform sampler2D texDiffuse0;

void main()
{
    float linearDepth = ToLinearPreviewDepth(textureLod(texDiffuse0, VAR._texCoord, lodLevel).r, zPlanes);
    _colourOut = vec4(vec3(linearDepth), 1.0);
}

-- Fragment.Layered

out vec4 _colourOut;
uniform float lodLevel = 0;

layout(binding = TEXTURE_UNIT0) uniform sampler2DArray texDiffuse0;
uniform int layer;

void main()
{
    _colourOut = textureLod(texDiffuse0, vec3(VAR._texCoord, layer), lodLevel);
    _colourOut.a = 1.0;
}

-- Fragment.Layered.LinearDepth

#include "utility.frag"

out vec4 _colourOut;


layout(binding = TEXTURE_UNIT0) uniform sampler2DArray texDiffuse0;
uniform int layer;
uniform float lodLevel = 0;
uniform vec2 zPlanes;

void main()
{
    float linearDepth = ToLinearPreviewDepth(textureLod(texDiffuse0, vec3(VAR._texCoord, layer), lodLevel).r, zPlanes);
    _colourOut = vec4(vec3(linearDepth), 1.0);
}

--Fragment.Layered.LinearDepth.ESM

#include "utility.frag"

out vec4 _colourOut;

layout(binding = TEXTURE_UNIT0) uniform sampler2DArray texDiffuse0;
uniform int layer;
uniform float lodLevel = 0.0;
uniform vec2 zPlanes;

void main()
{
    float depth = textureLod(texDiffuse0, vec3(VAR._texCoord, layer), lodLevel).r;
    //depth = 1.0 - (log(depth) / DEPTH_EXP_WARP);
	float linearDepth = ToLinearPreviewDepth(depth, zPlanes);
    _colourOut = vec4(vec3(linearDepth), 1.0);
}

--Fragment.Cube.LinearDepth

#include "utility.frag"

out vec4 _colourOut;

layout(binding = TEXTURE_UNIT0) uniform samplerCubeArrayShadow texDiffuse0;
uniform int layer;
uniform int face;
uniform vec2 zPlanes;

void main()
{

    float depth = texture(texDiffuse0, vec4(VAR._texCoord, face, layer), 1.0);
    //depth = 1.0 - (log(depth) / DEPTH_EXP_WARP);
    float linearDepth = ToLinearPreviewDepth(depth, zPlanes);
    _colourOut = vec4(vec3(linearDepth), 1.0);
}

--Fragment.Single.LinearDepth

#include "utility.frag"

out vec4 _colourOut;

layout(binding = TEXTURE_UNIT0) uniform sampler2DArrayShadow texDiffuse0;
uniform int layer;
uniform vec2 zPlanes;

void main()
{
    float depth = texture(texDiffuse0, vec4(VAR._texCoord, layer, 1.0)).r;
    float linearDepth = ToLinearPreviewDepth(depth, zPlanes);
    _colourOut = vec4(vec3(linearDepth), 1.0);
}