-- Vertex
#include "vertexDefault.vert"

void main(void)
{

    computeData();
}


-- Fragment

in  vec2 _texCoord;
out vec4 _colorOut;

uniform sampler2D texLeftEye;
uniform sampler2D texRightEye;

void main(void){
    vec4 colorLeftEye = texture(texLeftEye, _texCoord);
    vec4 colorRightEye = texture(texRightEye, _texCoord);
    _colorOut = vec4(colorLeftEye.r, colorRightEye.g, colorRightEye.b, 1.0);
}
