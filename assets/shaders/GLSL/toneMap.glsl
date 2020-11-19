-- Fragment

#include "utility.frag"

//ref: https://github.com/BruOp/bae

layout(binding = TEXTURE_UNIT0) uniform sampler2D texScreen;
layout(binding = TEXTURE_UNIT1) uniform sampler2D texExposure;

uniform float manualExposure = 4.975f;
uniform int mappingFunctions = UNCHARTED_2;
uniform bool useAdaptiveExposure = true;

out vec4 _colourOut;

float Reinhard(float x) {
    return x / (1.0f + x);
}

float Reinhard2(float x) {
    const float L_white = 4.0f;
    return (x * (1.0f + x / (L_white * L_white))) / (1.0 + x);
}

float Tonemap_ACES(float x) {
    // Narkowicz 2015, "ACES Filmic Tone Mapping Curve"
    const float a = 2.51;
    const float b = 0.03;
    const float c = 2.43;
    const float d = 0.59;
    const float e = 0.14;
    return (x * (a * x + b)) / (x * (c * x + d) + e);
}

float Tonemap_Unreal(float x) {
    // Unreal 3, Documentation: "Color Grading"
    // Adapted to be close to Tonemap_ACES, with similar range
    // Gamma 2.2 correction is baked in, don't use with sRGB conversion!
    return x / (x + 0.155) * 1.019;
}

float Tonemap_Uchimura(float x, float P, float a, float m, float l, float c, float b) {
    // Uchimura 2017, "HDR theory and practice"
    // Math: https://www.desmos.com/calculator/gslcdxvipg
    // Source: https://www.slideshare.net/nikuque/hdr-theory-and-practicce-jp
    float l0 = ((P - m) * l) / a;
    float L0 = m - m / a;
    float L1 = m + (1.0 - m) / a;
    float S0 = m + l0;
    float S1 = m + a * l0;
    float C2 = (a * P) / (P - S1);
    float CP = -C2 / P;
    float w0 = 1.0 - smoothstep(0.0, m, x);
    float w2 = step(m + l0, x);
    float w1 = 1.0 - w0 - w2;
    float T = m * pow(x / m, c) + b;
    float S = P - (P - S1) * exp(CP * (x - S0));
    float L = m + a * (x - m);
    return T * w0 + L * w1 + S * w2;
}

float Tonemap_Uchimura(float x) {
    const float P = 1.0;  // max display brightness
    const float a = 1.0;  // contrast
    const float m = 0.22; // linear section start
    const float l = 0.4;  // linear section length
    const float c = 1.33; // black
    const float b = 0.0;  // pedestal

    return Tonemap_Uchimura(x, P, a, m, l, c, b);
}

float Tonemap_Lottes(float x) {
    // Lottes 2016, "Advanced Techniques and Optimization of HDR Color Pipelines"
    const float a = 1.6;
    const float d = 0.977;
    const float hdrMax = 8.0;
    const float midIn = 0.18;
    const float midOut = 0.267;
    // Can be precomputed
    const float b =
        (-pow(midIn, a) + pow(hdrMax, a) * midOut) /
        ((pow(hdrMax, a * d) - pow(midIn, a * d)) * midOut);
    const float c =
        (pow(hdrMax, a * d) * pow(midIn, a) - pow(hdrMax, a) * pow(midIn, a * d) * midOut) /
        ((pow(hdrMax, a * d) - pow(midIn, a * d)) * midOut);
    return pow(x, a) / (pow(x, a * d) * b + c);
}

float Uncharted2(float x)
{
    const float A = 0.15;
    const float B = 0.50;
    const float C = 0.10;
    const float D = 0.20;
    const float E = 0.02;
    const float F = 0.30;
    const float W = 11.2f;

    x =  ((x*(A*x + C*B) + D*E) / (x*(A*x + B) + D*F)) - E / F;
    float white = ((W * (A * W + C * B) + D * E) / (W * (A * W + B) + D * F)) - E / F;
    
    return x / white;
}

vec3 convertRGB2Yxy(vec3 _rgb) {
    // Reference:
    // RGB/XYZ Matrices
    // http://www.brucelindbloom.com/index.html?Eqn_RGB_XYZ_Matrix.html
    vec3 xyz;
    xyz.x = dot(vec3(0.4124564, 0.3575761, 0.1804375), _rgb);
    xyz.y = dot(vec3(0.2126729, 0.7151522, 0.0721750), _rgb);
    xyz.z = dot(vec3(0.0193339, 0.1191920, 0.9503041), _rgb);

    // Reference:
    // http://www.brucelindbloom.com/index.html?Eqn_XYZ_to_xyY.html
    float inv = 1.0 / dot(xyz, vec3(1.0, 1.0, 1.0));
    return vec3(xyz.y, xyz.x * inv, xyz.y * inv);
}

vec3 convertYxy2RGB(vec3 _Yxy) {
    // Reference:
    // http://www.brucelindbloom.com/index.html?Eqn_xyY_to_XYZ.html
    vec3 xyz;
    xyz.x = _Yxy.x * _Yxy.y / _Yxy.z;
    xyz.y = _Yxy.x;
    xyz.z = _Yxy.x * (1.0 - _Yxy.y - _Yxy.z) / _Yxy.z;

    vec3 rgb;
    rgb.x = dot(vec3(3.2404542, -1.5371385, -0.4985314), xyz);
    rgb.y = dot(vec3(-0.9692660, 1.8760108, 0.0415560), xyz);
    rgb.z = dot(vec3(0.0556434, -0.2040259, 1.0572252), xyz);
    return rgb;
}

void main() {
    vec3 screenColour = texture(texScreen, VAR._texCoord).rgb * manualExposure;

    vec3 Yxy = convertRGB2Yxy(screenColour);
    float lp = Yxy.x;

    if (useAdaptiveExposure) {
        const float avgLuminance = texture(texExposure, VAR._texCoord).r;
        lp = Yxy.x / (9.6f * avgLuminance + 0.0001f);
    }

    if (mappingFunctions == REINHARD)
    {
        Yxy.x = Reinhard(lp);
    }
    else if (mappingFunctions == REINHARD_MODIFIED)
    {
        Yxy.x = Reinhard2(lp);
    }
    else if (mappingFunctions == GT)
    {
        Yxy.x = Tonemap_Uchimura(lp);
    }
    else if (mappingFunctions == ACES)
    {
        Yxy.x = Tonemap_ACES(lp);
    }
    else if (mappingFunctions == UNREAL_ACES)
    {
        Yxy.x = Tonemap_Unreal(lp);
    }
    else if (mappingFunctions == AMD_ACES)
    {
        Yxy.x = Tonemap_Lottes(lp);
    }
    else if (mappingFunctions == AMD_ACES)
    {
        Yxy.x = Tonemap_Uchimura(lp, 1.0f, 1.7f, 0.1f, 0.0f, 1.33f, 0.0f);
    }
    else if (mappingFunctions == UNCHARTED_2)
    {
        Yxy.x = Uncharted2(lp);
    }
    else
    {
        //Nothing. HDR? Nope. Straight up saturate (i.e. bad)
    }

    //Apply gamma correction here!
    _colourOut.rgb = ToSRGBAccurate(convertYxy2RGB(Yxy));
    _colourOut.a = luminance(_colourOut.rgb);
}