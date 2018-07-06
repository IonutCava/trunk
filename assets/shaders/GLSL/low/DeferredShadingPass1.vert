#version 150 compatibility
varying vec3 normals;
varying vec3 position;
varying vec4 texCoord[2];

void main( void ){
	gl_Position =  gl_ModelViewProjectionMatrix * gl_Vertex;

	texCoord[0] = gl_MultiTexCoord0;
	position = vec3(transpose(gl_ModelViewMatrix) * gl_Vertex);
	normals = normalize(gl_NormalMatrix * gl_Normal);
	
} 