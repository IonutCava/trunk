-- Vertex

void main(void)
{
    vec2 uv = vec2(0,0);
    if((gl_VertexID & 1) != 0)uv.x = 1;
    if((gl_VertexID & 2) != 0)uv.y = 1;

    VAR._texCoord = uv * 2;
    gl_Position.xy = uv * 4 - 1;
    gl_Position.zw = vec2(0,1);
}

--Fragment.BloomApply

layout(binding = TEXTURE_UNIT0) uniform sampler2D texScreen;
layout(binding = TEXTURE_UNIT1) uniform sampler2D texBloom;
uniform float bloomFactor;

out vec4 _colourOut;

void main() {
    _colourOut = texture(texScreen, VAR._texCoord);
    vec3 bloom = texture(texBloom, VAR._texCoord).rgb;
    _colourOut.rgb = 1.0 - ((1.0 - bloom * bloomFactor)*(1.0 - _colourOut.rgb));
}

-- Fragment.BloomCalc

#include "utility.frag"
out vec4 _bloomOut;

layout(binding = TEXTURE_UNIT0) uniform sampler2D texScreen;

void main() {    
    vec4 screenColour = texture(texScreen, VAR._texCoord);
    if (luminance(screenColour.rgb) > 1.0) {
        _bloomOut = screenColour;
    }
}
