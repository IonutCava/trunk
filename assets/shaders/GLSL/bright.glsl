-- Vertex
#include "vboInputData.vert"

void main(){
  computeData();
  gl_Position = gl_ModelViewProjectionMatrix * vertexData;
} 

-- Fragment

varying vec2 _texCoord;
uniform sampler2D texScreen;
uniform float threshold;

void main(){	
	vec4 value = texture(texScreen, _texCoord);
		
	if( (value.r + value.g + value.b)/3.0 > threshold )
		gl_FragColor = vec4(1.0, 1.0, 1.0, 1.0);
	else
		gl_FragColor = vec4(0.0, 0.0, 0.0, 1.0);

}
