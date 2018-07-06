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

layout(binding = TEXTURE_UNIT0) uniform sampler2D texScreen;

#ifdef USE_ADAPTIVE_LUMINANCE
layout(binding = TEXTURE_UNIT1) uniform sampler2D texExposure;
uniform int luminanceMipLevel;
uniform int exposureMipLevel;
#else
uniform float exposure = 0.975;
uniform float whitePoint = 0.975;
#endif

out vec4 _colourOut;

const float W = 11.2;
vec3 Uncharted2Tonemap(vec3 x)
{
    const float A = 0.15;
    const float B = 0.50;
    const float C = 0.10;
    const float D = 0.20;
    const float E = 0.02;
    const float F = 0.30;

    return ((x*(A*x + C*B) + D*E) / (x*(A*x + B) + D*F)) - E / F;
}

void main() {
    vec3 screenColour = texture(texScreen, VAR._texCoord).rgb;
#ifdef USE_ADAPTIVE_LUMINANCE
    float avgLuminance = exp(textureLod(texExposure, vec2(0.5, 0.5), exposureMipLevel).r);
    float exposure = sqrt(8.0 / (avgLuminance + 0.25));

    screenColour *= exposure;
    const float ExposureBias = 2.0f;
    vec3 curr = Uncharted2Tonemap(ExposureBias * screenColour);
    vec3 whiteScale = 1.0 / Uncharted2Tonemap(vec3(W));
    _colourOut.rgb = curr * whiteScale;
#else
    _colourOut.rgb = Uncharted2Tonemap(screenColour * exposure) / Uncharted2Tonemap(vec3(whitePoint));
#endif

    _colourOut.a = luminance(_colourOut.rgb);
}