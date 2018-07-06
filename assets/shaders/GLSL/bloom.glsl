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
    _colorOut.rgb += bloomFactor * texture(texBloom, VAR._texCoord).rgb;
}

-- Fragment.BloomCalc

out vec4 _bloomOut;

layout(binding = TEXTURE_UNIT0) uniform sampler2D texScreen;

void main() {    
    vec4 screenColor = texture(texScreen, VAR._texCoord);
    float luma = dot(screenColor.rgb, vec3(0.2126, 0.7152, 0.0722));
    if (luma > 1.0) {
        _bloomOut = screenColor;
    }
}

--Fragment.ToneMap

#include "utility.frag"
out vec4 _colorOut;

layout(binding = TEXTURE_UNIT0) uniform sampler2D texScreen;
layout(binding = TEXTURE_UNIT1) uniform sampler2D texExposure;

uniform int  exposureMipLevel;
uniform float whitePoint = 0.95;

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

const float KeyValue = 0.5;
// Determines the color based on exposure settings
vec3 CalcExposedColor(vec3 color, float avgLuminance, float threshold, out float exposure)
{
    // Use geometric mean
    avgLuminance = max(avgLuminance, 0.001f);
    float keyValue = KeyValue;
    float linearExposure = (KeyValue / avgLuminance);
    exposure = log(max(linearExposure, 0.0001f));
    exposure -= threshold;
    return exp(exposure) * color;
}

void main() {
    vec3 screenColor = texture(texScreen, VAR._texCoord).rgb;

    float avgLuminance = exp(textureLod(texExposure, vec2(0.5, 0.5), exposureMipLevel).r);
    float exposure = 0;
    float threshold = 0;
    screenColor = CalcExposedColor(screenColor, avgLuminance, threshold, exposure);
    _colorOut.rgb = ToSRGB(Uncharted2Tonemap(screenColor) / Uncharted2Tonemap(vec3(whitePoint)));
    _colorOut.a = dot(_colorOut.rgb, vec3(0.299, 0.587, 0.114));
}

--Fragment.LuminanceCalc

out float _colorOut;

layout(binding = TEXTURE_UNIT0) uniform sampler2D texScreen;
layout(binding = TEXTURE_UNIT1) uniform sampler2D texPrevExposure;

void main() {
    vec3 screenColor = texture(texScreen, VAR._texCoord).rgb;
    float luminance = dot(screenColor, vec3(0.299, 0.587, 0.114));
    float PreviousExposure = texture(texPrevExposure, VAR._texCoord).r;
    float TargetExposure = log(0.5 / clamp(dot(screenColor, vec3(0.299, 0.587, 0.114)), 0.3, 0.7));

    _colorOut = mix(PreviousExposure, TargetExposure, 0.1);
}