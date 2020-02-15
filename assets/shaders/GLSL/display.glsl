-- Fragment

#include "utility.frag"

layout(binding = TEXTURE_UNIT0) uniform sampler2D tex;
#if !defined(DEPTH_ONLY)
uniform uint convertToSRGB = 1u;
out vec4 _colourOut;
#endif

void main(void){
#if !defined(DEPTH_ONLY)
    vec4 colour = texture(tex, VAR._texCoord);
    if (convertToSRGB == 1u) {
        colour = ToSRGB(colour);
    }
    _colourOut = colour;
#else
    gl_FragDepth = texture(tex, VAR._texCoord).r;
#endif
}
