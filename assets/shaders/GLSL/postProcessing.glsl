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

uniform sampler2D texBruit;
uniform sampler2D texBruit2;

uniform bool enable_underwater;

uniform float _noiseTile;
uniform float _noiseFactor;
uniform float dvd_time;

#if defined(POSTFX_ENABLE_BLOOM)
uniform sampler2D texBloom;
uniform float bloom_factor;
#if defined(POSTFX_ENABLE_HDR)
uniform float exposure;
#endif
#endif

#if defined(POSTFX_ENABLE_SSAO)
uniform sampler2D texSSAO;
#endif

#if defined(POSTFX_ENABLE_DOF)
#endif

uniform bool  enable_vignette;
uniform bool  enable_noise;
uniform float randomCoeffNoise;
uniform float randomCoeffFlash;

const vec3 luminance = vec3 (0.299, 0.587, 0.114);

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
vec4 Bloom(in vec4 colorIn){

    vec4 blur = bloom_factor * texture(texBloom, _texCoord);
#if defined(POSTFX_ENABLE_HDR)
    float l = dot (luminance, colorIn);
    float scale = l / (1.0 + l);
    float alpha = colorIn.a;
    vec4 color = colorIn * scale + 5.0 * blur;
    color.a = alpha;
    return color;
#else
	return colorIn + blur;
#endif
}
#endif

vec4 LevelOfGrey(in vec4 colorIn)
{
	return vec4(colorIn.r * luminance.x, colorIn.g * luminance.y,colorIn.b * luminance.z,colorIn.a);
}

vec4 NoiseEffect(in vec4 colorIn)
{
	vec4 colorOut = LevelOfGrey(colorIn);
	vec4 colorNoise = texture(texBruit, _texCoord + vec2(randomCoeffNoise,randomCoeffNoise));
	colorOut = mix(colorNoise,vec4(1,1,1,1),randomCoeffFlash)/3.0 + 2.0*colorIn/3.0;

	return colorOut;
}

vec4 VignetteEffect(in vec4 colorIn)
{
	vec4 ColorVignette = texture(texVignette, _texCoord);
	
	vec4 colorOut = colorIn - (vec4(1,1,1,2)-ColorVignette);
	colorOut.r = clamp(colorOut.r,0.0,1.0);
	colorOut.g = clamp(colorOut.g,0.0,1.0);
	colorOut.b = clamp(colorOut.b,0.0,1.0);
	return colorOut;
}

vec4 UnderWater()
{
	vec4 colorOut;
	
	float time2 = dvd_time*0.0001;
	vec2 noiseUV = _texCoord*_noiseTile;
	vec2 uvNormal0 = noiseUV + time2;
	vec2 uvNormal1 = noiseUV;
	uvNormal1.s -= time2;
	uvNormal1.t = uvNormal0.t;
		
	vec3 normal0 = texture(texWaterNoiseNM, uvNormal0).rgb * 2.0 - 1.0;
	vec3 normal1 = texture(texWaterNoiseNM, uvNormal1).rgb * 2.0 - 1.0;
	vec3 normal = normalize(normal0+normal1);
	
	colorOut = clamp(texture(texScreen, _texCoord + _noiseFactor*normal0.st), 
				     vec4(0.0, 0.0, 0.0, 0.0),  
					 vec4(1.0, 1.0, 1.0, 1.0));
	
	return colorOut;
}


void main(void){
	
    _colorOut = texture(texScreen, _texCoord);

	if(enable_underwater)
		_colorOut = UnderWater();
	
#if defined(POSTFX_ENABLE_SSAO)
	_colorOut = SSAO(_colorOut);
#endif

#if defined(POSTFX_ENABLE_BLOOM)
	_colorOut = Bloom(_colorOut);
#endif

	if(enable_noise)
		_colorOut = NoiseEffect(_colorOut);
	
	if(enable_vignette)
		_colorOut = VignetteEffect(_colorOut);
}
