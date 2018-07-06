-- Vertex

varying vec3 normals;
varying float depth;

uniform float zNear;
uniform float zFar;

float LinearDepth(in float inDepth);

void main(void){
	gl_Position = ftransform();
	normals = normalize(gl_NormalMatrix * gl_Normal);
	vec4 vToEye = gl_ModelViewMatrix * gl_Vertex;	
	depth = LinearDepth(vToEye.z);
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