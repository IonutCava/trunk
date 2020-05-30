-- Vertex

#include "vertexDefault.vert"

void main(void)
{
    const NodeData data = fetchInputData();
    computeData(data);
    setClipPlanes(VAR._vertexW);
}

-- Fragment

out vec4 _colourOut;

layout(binding = TEXTURE_UNIT0) uniform sampler2D texDiffuse0;

void main()
{
    _colourOut = texture(texDiffuse0, VAR._texCoord);
}