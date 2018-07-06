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

uniform sampler2D texLeftEye;
uniform sampler2D texRightEye;

void main(void){
	vec4 colorLeftEye = texture2D(texLeftEye, texCoord[0]);
	vec4 colorRightEye = texture2D(texRightEye, texCoord[0]);
	gl_FragColor = vec4(colorLeftEye.r, colorRightEye.g, colorRightEye.b, 1.0);
}
