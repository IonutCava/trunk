-- Fragment

#define NEED_DEPTH_TEXTURE
#include "utility.frag"

out vec4 _colourOut;

layout(binding = TEX_BIND_POINT_SCREEN)     uniform sampler2D texScreen;
layout(binding = TEX_BIND_POINT_NOISE)      uniform sampler2D texNoise;
layout(binding = TEX_BIND_POINT_BORDER)     uniform sampler2D texVignette;
layout(binding = TEX_BIND_POINT_UNDERWATER) uniform sampler2D texWaterNoiseNM;

uniform float _noiseTile;
uniform float _noiseFactor;

uniform float randomCoeffNoise;
uniform float randomCoeffFlash;

// fade settings
uniform float _fadeStrength = 0.0;
uniform bool _fadeActive = false;
uniform vec4 _fadeColour;

uniform vec2 _zPlanes;

subroutine vec4 VignetteRoutineType(in vec4 colourIn);
subroutine vec4 NoiseRoutineType(in vec4 colourIn);
subroutine vec4 ScreenRoutineType();

layout(location = 0) subroutine uniform VignetteRoutineType VignetteRoutine;
layout(location = 1) subroutine uniform NoiseRoutineType NoiseRoutine;
layout(location = 2) subroutine uniform ScreenRoutineType ScreenRoutine;

vec4 LevelOfGrey(in vec4 colourIn) {
    return vec4(colourIn.r * 0.299, colourIn.g * 0.587, colourIn.b * 0.114, colourIn.a);
}

subroutine(VignetteRoutineType)
vec4 Vignette(in vec4 colourIn){
    vec4 colourOut = colourIn - (vec4(1,1,1,2) - texture(texVignette, VAR._texCoord));
    return vec4(clamp(colourOut.rgb,0.0,1.0), colourOut.a);
}

subroutine(NoiseRoutineType)
vec4 Noise(in vec4 colourIn){
    return mix(texture(texNoise, VAR._texCoord + vec2(randomCoeffNoise, randomCoeffNoise)),
               vec4(1.0), randomCoeffFlash) / 3.0 + 2.0 * LevelOfGrey(colourIn) / 3.0;
}

subroutine(ScreenRoutineType)
vec4 screenUnderwater(){
    float time2 = float(dvd_time) * 0.00001;
    vec2 uvNormal0 = VAR._texCoord * _noiseTile;
    uvNormal0.s += time2;
    uvNormal0.t += time2;
    vec2 uvNormal1 = VAR._texCoord * _noiseTile;
    uvNormal1.s -= time2;
    uvNormal1.t += time2;

    vec3 normal0 = texture(texWaterNoiseNM, uvNormal0).rgb * 2.0 - 1.0;
    vec3 normal1 = texture(texWaterNoiseNM, uvNormal1).rgb * 2.0 - 1.0;
    vec3 normal = normalize(normal0 + normal1);

    vec2 coords = VAR._texCoord + (normal.xy * _noiseFactor);
    return clamp(texture(texScreen, coords) * vec4(0.35), vec4(0.0), vec4(1.0));
}

subroutine(ScreenRoutineType)
vec4 screenNormal(){
    return texture(texScreen, VAR._texCoord);
}

subroutine(NoiseRoutineType, VignetteRoutineType)
vec4 ColourPassThrough(in vec4 colourIn){
    return colourIn;
}

void main(void){
    vec4 colour = VignetteRoutine(NoiseRoutine(ScreenRoutine()));
    if (_fadeActive) {
        colour = mix(colour, _fadeColour, _fadeStrength);
    }

    float depth = getDepthValue(getScreenPositionNormalised());
    _colourOut = applyFog(depth, colour, _zPlanes);
    //_colourOut = screenNormal();
}
