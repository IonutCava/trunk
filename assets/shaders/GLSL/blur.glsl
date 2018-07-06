-- Vertex
#include "vboInputData.vert"
varying vec2 texCoord[2];

void main(void){
    computeData();
    texCoord[0] = texCoordData;
    gl_Position = gl_ModelViewProjectionMatrix * vertexData;
} 

-- Fragment

varying vec2 texCoord[2];
uniform sampler2D texScreen;
uniform vec2 size;
uniform bool horizontal;
uniform int kernel_size;

void main(){

	vec2 pas = 1.0/size;
	vec2 uv = texCoord[0];
	vec4 color = vec4(0.0, 0.0, 0.0, 1.0);
	float j = 0;
	
	// HORIZONTAL BLUR
	if(horizontal) {
		int j = 0;
		int sum = 0;
		for(int i=-kernel_size; i<=kernel_size; i++) {
			vec4 value = texture2D(texScreen, uv + vec2(pas.x*i, 0.0));
			j = i;
			int factor = kernel_size+1 - abs(j);
			color += value * factor;
			sum += factor;
		}
		color /= sum;
	}
	
	// VERTICAL BLUR
	else {
		int j = 0;
		int sum = 0;
		
		for(int i=-kernel_size; i<=kernel_size; i++) {
			vec4 value = texture2D(texScreen, uv + vec2(0.0, pas.y*i));
			j = i;
			int factor = kernel_size+1 - abs(j);
			color += value * factor;
			sum += factor;
		}
		color /= sum;
	}
	
	gl_FragColor = color;
	gl_FragColor.a = 1.0;
}