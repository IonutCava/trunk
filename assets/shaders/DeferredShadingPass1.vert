#version 150 compatibility
varying vec3 normals;
varying vec3 position;
uniform mat4 transformMatrix;
uniform mat4 parentTransformMatrix;

void main( void )
{
	gl_Position =  gl_ModelViewProjectionMatrix * gl_Vertex;
	gl_TexCoord[0] = gl_MultiTexCoord0;
	position = vec3(transpose(gl_ModelViewMatrix) * gl_Vertex);
	normals = normalize(gl_NormalMatrix * gl_Normal);
	
	//gl_FrontColor = vec4(1.0, 1.0, 1.0, 1.0);

} 