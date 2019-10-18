-- Fragment

#include "utility.frag"

layout(binding = TEXTURE_UNIT0) uniform sampler2D tex;
uniform bool srgb = true;

out vec4 _colourOut;

void main(void){
    vec4 colour = texture(tex, VAR._texCoord);
    if (srgb) {
        colour = ToSRGB(colour);
    }
    _colourOut = colour;
}
