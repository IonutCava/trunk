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

-- Fragment
#include "utility.frag"
out vec4 _colorOut;

layout(binding = TEXTURE_UNIT0) uniform sampler2D texScreen;
layout(binding = TEXTURE_UNIT1) uniform sampler2D texExposure;

uniform int  exposureMipLevel;
uniform bool toneMap = false;
uniform float whitePoint = 0.92;

vec3 Uncharted2Tonemap(vec3 x)
{
    float A = 0.15;
    float B = 0.50;
    float C = 0.10;
    float D = 0.20;
    float E = 0.02;
    float F = 0.30;

    return ((x*(A*x+C*B)+D*E)/(x*(A*x+B)+D*F))-E/F;
}

void main() {    
    vec3 screenColor = texture(texScreen, VAR._texCoord).rgb;

    if(toneMap) {
        float exposure = exp(textureLod(texExposure, vec2(0.5,0.5), exposureMipLevel).r);
        _colorOut = vec4(ToSRGB(Uncharted2Tonemap(screenColor * exposure) / Uncharted2Tonemap(vec3(whitePoint))) , 1.0);
    }else{
        if (dot(screenColor.rgb, vec3(0.2126, 0.7152, 0.0722)) > whitePoint) {
            _colorOut.rgb = screenColor;
        }
    }

}
