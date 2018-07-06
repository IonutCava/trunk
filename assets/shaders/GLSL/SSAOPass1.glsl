-- Vertex
#include "vboInputData.vert"
out vec3 normals;
out float depth;

uniform vec2 zPlanes;

float LinearDepth(in float inDepth);

void main(void){
	computeData();
	normals = normalize(dvd_NormalMatrix * dvd_Normal);
	vec4 vToEye = dvd_ModelViewMatrix * dvd_Vertex;	
	depth = LinearDepth(vToEye.z);
	gl_Position = dvd_ModelViewProjectionMatrix * dvd_Vertex;
}

float LinearDepth(in float inDepth){
	float dif = zPlanes.y - zPlanes.x;
	float A = -(zPlanes.y + zPlanes.x) / dif;
	float B = -2*zPlanes.y*zPlanes.x / dif;
	float C = -(A*inDepth + B) / inDepth; // C in [-1, 1]
	return 0.5 * C + 0.5; // in [0, 1]
}


-- Fragment

in vec3 normals;
in float depth;
out vec4 _colorOut;

void main(void){

   _colorOut = vec4(normalize(normals),depth);
}