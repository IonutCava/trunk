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

-- Fragment.Layered

in vec2 _texCoord;
out vec4 _colorOut;

uniform sampler2DArray tex;
uniform int layer;

void main()
{
    _colorOut = texture(tex, vec3(_texCoord, layer));
}