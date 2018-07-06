
out vec2 _texCoord;

void computeData()
{
	_texCoord = inTexCoordData;
	gl_Position = dvd_ViewProjectionMatrix * dvd_WorldMatrix[dvd_drawID] * vec4(inVertexData,1.0);
}