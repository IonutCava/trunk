varying vec3 normals;
varying float depth;

uniform float zNear;
uniform float zFar;

void main(void){
	gl_Position = ftransform();

	normals = normalize(gl_NormalMatrix * gl_Normal);
	// this will transform the vertex into eyespace
	vec4 viewPos = gl_ModelViewMatrix * gl_Vertex; 
	// minus because in OpenGL we are looking in the negative z-direction
	// will map near..far to 0..1
	depth = (-viewPos.z-zNear)/(zFar-zNear);
}