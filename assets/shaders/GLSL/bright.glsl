-- Vertex
#include "vertexDefault.vert"

void main(void)
{
	computeData();
}

-- Fragment

in vec2  _texCoord;
out vec4 _colorOut;

uniform sampler2D texScreen;
uniform float whitePoint = 0.95;

void main(){	
	vec3 value = texture(texScreen, _texCoord).rgb;

	_colorOut = clamp(sign(((value.r + value.g + value.b)/3.0) - whitePoint) * vec4(1.0), 0.0, 1.0);
}
