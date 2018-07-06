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
    _colorOut.rgb = mix(vec3(1.0,0.0,0.0), vec3(0.0,0.0,1.0), linearDepth);
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

void main()
{
    float n = dvd_zPlanes.x;
    float f = dvd_zPlanes.y;
    float depth = texture(tex, vec3(_texCoord, layer)).r;
    float linearDepth = (2 * n) / (f + n - (depth) * (f - n));
    _colorOut.rgb = vec3(linearDepth);
}