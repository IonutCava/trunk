-- Vertex
#include "vboInputData.vert"

void main(void){
	computeData();
	gl_Position = gl_ModelViewProjectionMatrix * vertexData;
}


-- Fragment

varying vec2 _texCoord;

uniform sampler2D texLeftEye;
uniform sampler2D texRightEye;

void main(void){
	vec4 colorLeftEye = texture(texLeftEye, _texCoord);
	vec4 colorRightEye = texture(texRightEye, _texCoord);
	gl_FragColor = vec4(colorLeftEye.r, colorRightEye.g, colorRightEye.b, 1.0);
}
