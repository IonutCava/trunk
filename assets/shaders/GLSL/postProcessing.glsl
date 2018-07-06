-- Vertex 

#include "vertexDefault.vert"

void main(void)
{

	computeData();
}

-- Fragment

in  vec2 _texCoord;
out vec4 _colorOut;

uniform sampler2D texScreen;
uniform sampler2D texVignette;
uniform sampler2D texWaterNoiseNM;

uniform sampler2D texNoise;

uniform bool enable_underwater;

#ifdef USE_DEPTH_PREVIEW
uniform vec2 dvd_zPlanes;
uniform bool enable_depth_preview;
#endif

uniform float _noiseTile;
uniform float _noiseFactor;
uniform float dvd_time;

#if defined(POSTFX_ENABLE_BLOOM)
uniform sampler2D texBloom;
uniform float bloomFactor;
#endif

#if defined(POSTFX_ENABLE_SSAO)
uniform sampler2D texSSAO;
#endif

uniform bool  enableVignette;
uniform bool  enableNoise;
uniform float randomCoeffNoise;
uniform float randomCoeffFlash;

#if defined(POSTFX_ENABLE_SSAO)
vec4 SSAO(in vec4 color){
	float ssaoFilter = texture(texSSAO, _texCoord).r;
	if(ssaoFilter > 0){
		color.rgb = color.rgb * ssaoFilter;
	}
	return color;
}
#endif

#if defined(POSTFX_ENABLE_BLOOM)
vec4 Bloom(in vec4 colorIn) {
	return colorIn + bloomFactor * texture(texBloom, _texCoord);
}
#endif

vec4 LevelOfGrey(in vec4 colorIn) {

	return vec4(colorIn.r * 0.299, colorIn.g * 0.587, colorIn.b * 0.114, colorIn.a);
}

vec4 NoiseEffect(in vec4 colorIn) {

	return mix(texture(texNoise, _texCoord + vec2(randomCoeffNoise, randomCoeffNoise)), 
		       vec4(1,1,1,1),
			   randomCoeffFlash) / 3.0 + 2.0 * LevelOfGrey(colorIn) / 3.0;
}

vec4 VignetteEffect(in vec4 colorIn) {

	vec4 colorOut = colorIn - (vec4(1,1,1,2) - texture(texVignette, _texCoord));
	return vec4(clamp(colorOut.rgb,0.0,1.0), colorOut.a);
}

vec4 UnderWater() {

	float time2 = dvd_time*0.0001;
	vec2 noiseUV = _texCoord*_noiseTile;
	vec2 uvNormal0 = noiseUV + time2;
	vec2 uvNormal1 = noiseUV;
	uvNormal1.s -= time2;
	uvNormal1.t = uvNormal0.t;
		
	vec3 normal0 = 2.0 * texture(texWaterNoiseNM, uvNormal0).rgb - 1.0;
	vec3 normal1 = 2.0 * texture(texWaterNoiseNM, uvNormal1).rgb - 1.0;
	vec3 normal = normalize(normal0+normal1);
	
	return clamp(texture(texScreen, _texCoord + _noiseFactor*normal0.st), 0.0, 1.0);
}


void main(void){

#if defined(USE_DEPTH_PREVIEW)
    if(enable_depth_preview){
        float z = (2 * dvd_zPlanes.x) / (dvd_zPlanes.y +dvd_zPlanes.x - texture(texScreen, _texCoord).r * (dvd_zPlanes.y - dvd_zPlanes.x));
        _colorOut = vec4(z, z, z, 1.0);
    }else{
#endif

    if(enable_underwater)
		_colorOut = UnderWater();
	else 
        _colorOut = texture(texScreen, _texCoord);

#if defined(POSTFX_ENABLE_SSAO)
	_colorOut = SSAO(_colorOut);
#endif

#if defined(POSTFX_ENABLE_BLOOM)
	_colorOut = Bloom(_colorOut);
#endif

	if(enableNoise)
		_colorOut = NoiseEffect(_colorOut);
	
	if(enableVignette)
		_colorOut = VignetteEffect(_colorOut);
   
#if defined(USE_DEPTH_PREVIEW)
    }
#endif
}
