-- Vertex

#include "vertexDefault.vert"

void main(void)
{
    computeData();
}

-- Fragment

in vec2 _texCoord;
out vec4 _colorOut;

uniform sampler2D tex;

void main()
{
    _colorOut = texture(tex, _texCoord);
    _colorOut.a = 1.0;
}

-- Fragment.LinearDepth

in vec2 _texCoord;
out vec4 _colorOut;

uniform sampler2D tex;
uniform vec2 dvd_zPlanes;

void main()
{
    float n = dvd_zPlanes.x;
    float f = dvd_zPlanes.y * 0.5;
    float depth = texture(tex, _texCoord).r;
    float linearDepth = (2 * n) / (f + n - (depth) * (f - n));
    _colorOut.rgb = vec3(linearDepth);
    _colorOut.a = 1.0;
}

-- Fragment.Layered

in vec2 _texCoord;
out vec4 _colorOut;

uniform sampler2DArray tex;
uniform int layer;

void main()
{
    _colorOut = texture(tex, vec3(_texCoord, layer));
}

-- Fragment.Layered.LinearDepth

in vec2 _texCoord;
out vec4 _colorOut;

uniform sampler2DArray tex;
uniform int layer;
uniform vec2 dvd_zPlanes;
uniform float far_plane = 1000.0;

void main()
{
    float n = dvd_zPlanes.x;
    float f = min(dvd_zPlanes.y, far_plane);
    float depth = texture(tex, vec3(_texCoord, layer)).r;
    _colorOut.rgb = vec3((2 * n) / (f + n - (depth)* (f - n)));
}

--Fragment.Layered.LinearDepth.ESM

in vec2 _texCoord;
out vec4 _colorOut;

uniform sampler2DArray tex;
uniform int layer;
uniform vec2 zPlanes;

void main()
{
    float depth = texture(tex, vec3(_texCoord, layer)).r;
    //depth = 1.0 - (log(depth) / DEPTH_EXP_WARP);
    
    float n = zPlanes.x;
    float f = zPlanes.y;

    float linearDepth = (2 * n) / (f + n - (depth)* (f - n));
    _colorOut.rgb = vec3(linearDepth);
}