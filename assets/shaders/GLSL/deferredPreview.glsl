-- Vertex

#include "vboInputData.vert"

void main()
{
	computeData();
	gl_Position = gl_ModelViewProjectionMatrix * vertexData;
}

-- Fragment

varying vec2 _texCoord;

uniform sampler2D tex;

void main()
{
	gl_FragColor = texture(tex, _texCoord);
}