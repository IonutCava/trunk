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

#define THRESHOLD 0.95

void main(){	
	vec3 value = texture(texScreen, _texCoord).rgb;
	float selection = clamp(sign(((value.r + value.g + value.b)/3.0) - THRESHOLD),0.0,1.0);

	_colorOut = mix(vec4(0.0, 0.0, 0.0, 0.0),
                    vec4(1.0, 1.0, 1.0, 1.0),
                    selection);


}
