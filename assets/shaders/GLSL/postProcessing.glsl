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
    float time2 = float(dvd_time) * 0.0001;
    vec2 noiseUV = VAR._texCoord * _noiseTile;
    vec2 uvNormal0 = noiseUV + time2;
    vec2 uvNormal1 = noiseUV;
    uvNormal1.s -= time2;
    uvNormal1.t += time2;
        
    vec3 normal0 = texture(texWaterNoiseNM, uvNormal0).rgb;
    vec3 normal1 = texture(texWaterNoiseNM, uvNormal1).rgb;
    vec3 normal = normalize(2.0 * ((normal0 + normal1) * 0.5) - 1.0);
    
    return clamp(texture(texScreen, VAR._texCoord + _noiseFactor * normal.st), vec4(0.0), vec4(1.0));
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

    float depth = textureLod(texDepthMap, getScreenPositionNormalised(), 0).r;
    _colourOut = applyFog(depth, colour);
    //_colourOut = screenNormal();
}
