-- Fragment

#include "utility.frag"

out vec4 _colourOut;

layout(binding = TEX_BIND_POINT_SCREEN)     uniform sampler2D texScreen;
layout(binding = TEX_BIND_POINT_NOISE)      uniform sampler2D texNoise;
layout(binding = TEX_BIND_POINT_BORDER)     uniform sampler2D texVignette;
layout(binding = TEX_BIND_POINT_UNDERWATER) uniform sampler2D texWaterNoiseNM;
layout(binding = TEXTURE_DEPTH_MAP)         uniform sampler2D texDepthMap;

uniform float _noiseTile;
uniform float _noiseFactor;

uniform float randomCoeffNoise;
uniform float randomCoeffFlash;


uniform bool vignetteEnabled = false;
uniform bool noiseEnabled = false;
uniform bool underwaterEnabled = false;

// fade settings
uniform float _fadeStrength = 0.0;
uniform bool _fadeActive = false;
uniform vec4 _fadeColour;

uniform vec2 _zPlanes;

vec4 LevelOfGrey(in vec4 colourIn) {
    return vec4(colourIn.r * 0.299f, colourIn.g * 0.587f, colourIn.b * 0.114f, colourIn.a);
}

vec4 Vignette(in vec4 colourIn){
    vec4 colourOut = colourIn - (vec4(1,1,1,2) - texture(texVignette, VAR._texCoord));
    return vec4(clamp(colourOut.rgb,0.0,1.0), colourOut.a);
}

vec4 Noise(in vec4 colourIn){
    return mix(texture(texNoise, VAR._texCoord + vec2(randomCoeffNoise, randomCoeffNoise)),
               vec4(1.0), randomCoeffFlash) / 3.0f + 2.0f * LevelOfGrey(colourIn) / 3.0f;
}

vec4 Underwater() {
    float time2 = float(dvd_time) * 0.00001;
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

    const float depth = textureLod(texDepthMap, dvd_screenPositionNormalised, 0).r;
    _colourOut = applyFog(depth, colour, _zPlanes);
}
