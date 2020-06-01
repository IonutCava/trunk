-- Vertex

#include "vertexDefault.vert"

void main(void)
{
    computeData(fetchInputData());
    setClipPlanes();
}

-- Fragment

out vec4 _colourOut;

layout(binding = TEXTURE_UNIT0) uniform sampler2D texDiffuse0;

void main()
{
    _colourOut = texture(texDiffuse0, VAR._texCoord);
}