-- Fragment

#include "utility.frag"

layout(binding = TEXTURE_UNIT0) uniform sampler2D tex;

out vec4 _colourOut;

void main(void){
    _colourOut = ToSRGB(texture(tex, VAR._texCoord));
}
