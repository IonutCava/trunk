-- Vertex

#include "vertexDefault.vert"

void main(void)
{

	computeData();
}

-- Fragment

in  vec2 _texCoord;
out vec4 _colorOut;

uniform sampler2D tex;

void main()
{
	_colorOut = texture(tex, _texCoord);
}