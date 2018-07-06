-- Vertex
#include "vertexDefault.vert"

void main(void)
{
	computeData();
}

-- Fragment

in  vec2 _texCoord;
out vec4 _colorOut;

uniform sampler2D texScreen;
uniform vec2 size;
uniform bool horizontal;
uniform int kernelSize;

void main(){

	const vec2 pass = 1.0/size;
	vec3 color = vec3(0.0);
	vec3 value;
	int i = 0;
	int sum = 0;
	int factor = 0;
	// HORIZONTAL BLUR
	if(horizontal) {

		for(i = -kernelSize; i <= kernelSize; i++) {
			value = texture(texScreen, _texCoord + vec2(pass.x*i, 0.0)).rgb;
			factor = kernelSize+1 - abs(i);
			color += value * factor;
			sum += factor;
		}
	} else {// VERTICAL BLUR

		for(i = -kernelSize; i <= kernelSize; i++) {
			value = texture(texScreen, _texCoord + vec2(0.0, pass.y*i)).rgb;
			factor = kernelSize+1 - abs(i);
			color += value * factor;
			sum += factor;
		}
	}

	_colorOut = vec4(color / sum,1.0);
}
