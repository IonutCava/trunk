
uniform mat4 dvd_WorldViewProjectionMatrix;

out vec2 _texCoord;

void computeData()
{
	_texCoord = inTexCoordData;
	gl_Position = dvd_WorldViewProjectionMatrix * vec4(inVertexData,1.0);
}