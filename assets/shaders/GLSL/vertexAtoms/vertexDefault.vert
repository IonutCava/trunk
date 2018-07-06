in vec3  inVertexData;
in vec2  inTexCoordData;
uniform mat4 dvd_ModelViewProjectionMatrix;

out vec2 _texCoord;
void computeData()
{
	_texCoord = inTexCoordData;
	gl_Position = dvd_ModelViewProjectionMatrix * vec4(inVertexData,1.0);
}