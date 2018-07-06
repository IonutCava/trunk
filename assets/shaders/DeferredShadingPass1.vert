#version 150 compatibility
varying vec3 normals;
varying vec3 position;

uniform mat4 modelViewProjectionMatrix;
uniform mat4 modelViewMatrix;

void main( void ){
	gl_Position =  gl_ModelViewProjectionMatrix * gl_Vertex;

	gl_TexCoord[0] = gl_MultiTexCoord0;
	position = vec3(transpose(modelViewMatrix) * gl_Vertex);
	normals = normalize(gl_NormalMatrix * gl_Normal);
	
} 