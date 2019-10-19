-- Fragment

#include "utility.frag"

layout(binding = TEXTURE_UNIT0) uniform sampler2D tex;
uniform uint convertToSRGB = 1u;

out vec4 _colourOut;

void main(void){
    vec4 colour = texture(tex, VAR._texCoord);
    if (convertToSRGB == 1) {
        colour = ToSRGB(colour);
    }
    _colourOut = colour;
}
