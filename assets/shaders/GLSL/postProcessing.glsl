-- Fragment

#include "utility.frag"

#define NEED_SCENE_DATA
#include "sceneData.cmn"
layout(location = TARGET_ALBEDO) out vec4 _colourOut;

layout(binding = TEX_BIND_POINT_SCREEN)     uniform sampler2D texScreen;
layout(binding = TEX_BIND_POINT_NOISE)      uniform sampler2D texNoise;
layout(binding = TEX_BIND_POINT_BORDER)     uniform sampler2D texVignette;
layout(binding = TEX_BIND_POINT_UNDERWATER) uniform sampler2D texWaterNoiseNM;
layout(binding = TEX_BIND_POINT_POSTFXDATA) uniform sampler2D texPostFXData;
layout(binding = TEX_BIND_POINT_SSR)        uniform sampler2D texSSR;

uniform vec4 _fadeColour;
uniform vec2 _zPlanes;
uniform float _noiseTile;
uniform float _noiseFactor;
uniform float randomCoeffNoise;
uniform float randomCoeffFlash;
uniform float _fadeStrength;
uniform bool vignetteEnabled;
uniform bool noiseEnabled;
uniform bool underwaterEnabled;
uniform bool _fadeActive;

vec4 Vignette(in vec4 colourIn){
    vec4 colourOut = colourIn - (vec4(1,1,1,2) - texture(texVignette, VAR._texCoord));
    return vec4(clamp(colourOut.rgb,0.0,1.0), colourOut.a);
}

#define LevelOfGrey(C) vec4(C.rgb * _kLum, C.a)
vec4 Noise(in vec4 colourIn){
    return mix(texture(texNoise, VAR._texCoord + vec2(randomCoeffNoise, randomCoeffNoise)),
               vec4(1.0), randomCoeffFlash) / 3.0f + 2.0f * LevelOfGrey(colourIn) / 3.0f;
}

vec4 Underwater() {
    float time2 = MSToSeconds(dvd_time) * 0.01f;
    vec2 uvNormal0 = VAR._texCoord * _noiseTile;
    uvNormal0.s += time2;
    uvNormal0.t += time2;
    vec2 uvNormal1 = VAR._texCoord * _noiseTile;
    uvNormal1.s -= time2;
    uvNormal1.t += time2;

    vec3 normal0 = texture(texWaterNoiseNM, uvNormal0).rgb * 2.0f - 1.0f;
    vec3 normal1 = texture(texWaterNoiseNM, uvNormal1).rgb * 2.0f - 1.0f;
    vec3 normal = normalize(normal0 + normal1);

    vec2 coords = VAR._texCoord + (normal.xy * _noiseFactor);
    return clamp(texture(texScreen, coords) * vec4(0.35), vec4(0.0), vec4(1.0));
}

void main(void){
    switch (dvd_materialDebugFlag) {
        case DEBUG_DEPTH:          _colourOut = vec4(vec3(texture(texPostFXData, VAR._texCoord).g / _zPlanes.y), 1.0f); return;
        case DEBUG_SSAO:           _colourOut = vec4(vec3(texture(texPostFXData, VAR._texCoord).r), 1.0f); return;
        case DEBUG_SSR:            _colourOut = vec4(texture(texSSR, VAR._texCoord).rgb, 1.0f); return;
    }

    vec4 colour = underwaterEnabled ? Underwater() : texture(texScreen, VAR._texCoord);
    if (noiseEnabled) {
        colour = Noise(colour);
    }
    if (vignetteEnabled) {
        colour = Vignette(colour);
    }

    if (_fadeActive) {
        colour = mix(colour, _fadeColour, _fadeStrength);
    }

    _colourOut = colour;
}
