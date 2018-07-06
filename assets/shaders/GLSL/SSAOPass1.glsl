-- Vertex
#include "vboInputData.vert"
varying vec3 normals;
varying float depth;

uniform float zNear;
uniform float zFar;

float LinearDepth(in float inDepth);

void main(void){
	computeData();
	normals = normalize(gl_NormalMatrix * normalData);
	vec4 vToEye = gl_ModelViewMatrix * vertexData;	
	depth = LinearDepth(vToEye.z);
	gl_Position = gl_ModelViewProjectionMatrix * vertexData;
}

float LinearDepth(in float inDepth){
	float dif = zFar - zNear;
	float A = -(zFar + zNear) / dif;
	float B = -2*zFar*zNear / dif;
	float C = -(A*inDepth + B) / inDepth; // C in [-1, 1]
	return 0.5 * C + 0.5; // in [0, 1]
}


-- Fragment

varying vec3 normals;
varying float depth;

void main(void){

   gl_FragData[0] = vec4(normalize(normals),depth);
}