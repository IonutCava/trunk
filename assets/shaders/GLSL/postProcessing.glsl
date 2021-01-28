-- Fragment

#include "utility.frag"

#define NEED_SCENE_DATA
#include "sceneData.cmn"
out vec4 _colourOut;

layout(binding = TEX_BIND_POINT_SCREEN)     uniform sampler2D texScreen;
layout(binding = TEX_BIND_POINT_GBUFFER)    uniform sampler2D texGBuffer;
layout(binding = TEX_BIND_POINT_NOISE)      uniform sampler2D texNoise;
layout(binding = TEX_BIND_POINT_BORDER)     uniform sampler2D texVignette;
layout(binding = TEX_BIND_POINT_UNDERWATER) uniform sampler2D texWaterNoiseNM;
layout(binding = TEXTURE_DEPTH_MAP)         uniform sampler2D texDepthMap;

ADD_UNIFORM(mat4, invViewMatrix);
ADD_UNIFORM(mat4, invProjectionMatrix);
ADD_UNIFORM(vec4, _fadeColour);
ADD_UNIFORM(vec3, camPosition);
ADD_UNIFORM(vec2, _zPlanes);
ADD_UNIFORM(float, _noiseTile);
ADD_UNIFORM(float, _noiseFactor);
ADD_UNIFORM(float, randomCoeffNoise);
ADD_UNIFORM(float, randomCoeffFlash);
ADD_UNIFORM(float, _fadeStrength);
ADD_UNIFORM(bool, vignetteEnabled);
ADD_UNIFORM(bool, noiseEnabled);
ADD_UNIFORM(bool, underwaterEnabled);
ADD_UNIFORM(bool, _fadeActive);

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
