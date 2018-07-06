-- Vertex

#include "vertexDefault.vert"

void main(void)
{

    computeData();
}

-- Fragment

out vec4 _colorOut;

layout(binding = TEXTURE_UNIT0) uniform sampler2D texDiffuse0;

void main()
{
    _colorOut = texture(texDiffuse0, VAR._texCoord);
}