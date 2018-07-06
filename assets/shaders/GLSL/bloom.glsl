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

out vec4 _colorOut;

void main() {
    _colorOut = texture(texScreen, VAR._texCoord);
    vec3 bloom = texture(texBloom, VAR._texCoord).rgb;
    _colorOut.rgb = 1.0 - ((1.0 - bloom * bloomFactor)*(1.0 - _colorOut.rgb));
}

-- Fragment.BloomCalc

#include "utility.frag"
out vec4 _bloomOut;

layout(binding = TEXTURE_UNIT0) uniform sampler2D texScreen;

void main() {    
    vec4 screenColor = texture(texScreen, VAR._texCoord);
    float luma = luminance(screenColor.rgb);
    if (luma > 1.0) {
        _bloomOut = screenColor;
    }
}

--Fragment.ToneMap

#include "utility.frag"
out vec4 _colorOut;

layout(binding = TEXTURE_UNIT0) uniform sampler2D texScreen;

#ifdef USE_ADAPTIVE_LUMINANCE
layout(binding = TEXTURE_UNIT1) uniform sampler2D texExposure;
uniform int luminanceMipLevel;
uniform int exposureMipLevel;
#else
uniform float exposure = 0.975;
uniform float whitePoint = 0.975;
#endif

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
    vec3 screenColor = texture(texScreen, VAR._texCoord).rgb;
#ifdef USE_ADAPTIVE_LUMINANCE
    float avgLuminance = exp(textureLod(texExposure, vec2(0.5, 0.5), exposureMipLevel).r);
    float exposure = sqrt(8.0 / (avgLuminance + 0.25));

    screenColor *= exposure;
    const float ExposureBias = 2.0f;
    vec3 curr = Uncharted2Tonemap(ExposureBias * screenColor);
    vec3 whiteScale = 1.0 / Uncharted2Tonemap(vec3(W));
    _colorOut.rgb = ToSRGB(curr * whiteScale);
#else
    _colorOut.rgb = ToSRGB(Uncharted2Tonemap(screenColor * exposure) / Uncharted2Tonemap(vec3(whitePoint)));
#endif

    _colorOut.a = luminance(_colorOut.rgb);
}

--Fragment.LuminanceCalc

#include "utility.frag"

out float _colorOut;

layout(binding = TEXTURE_UNIT0) uniform sampler2D texScreen;
layout(binding = TEXTURE_UNIT1) uniform sampler2D texPrevLuminance;

void main() {
    //Human eye:
    // - bright sunlight -> complete dark: 20-30 min
    // - complete dark -> bright sunlight: 5 min
    // - speed diff ~5
    vec3 screenColor = texture(texScreen, VAR._texCoord).rgb;
    float crtluminance = clamp(luminance(screenColor), 0.35, 0.75);
    float prevLuminance = clamp(exp(texture(texPrevLuminance, VAR._texCoord).r), 0.35, 0.75);
    // Slowly change luminance by mimicking human eye behaviour:
    // bright->dark 5 times faster than dark->bright
    float tau = mix(0.15, 0.35, prevLuminance > crtluminance);
    _colorOut = prevLuminance + (crtluminance - prevLuminance) * (1 - exp(-dvd_deltaTime() * tau));
}