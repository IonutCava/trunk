-- Fragment

#if !defined(DEPTH_ONLY)
#include "utility.frag"
#endif

layout(binding = TEXTURE_UNIT0) uniform sampler2D tex;
#if !defined(DEPTH_ONLY)
ADD_UNIFORM(uint, convertToSRGB)
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
