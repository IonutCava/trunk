-- Vertex 

varying vec2 vextexDataTramsformed;

#include "vboInputData.vert"

void main(void){
	computeData();
	vextexDataTramsformed = (vec2( vertexData.x, - vertexData.y ) + vec2( 1.0 ) ) * 0.5;
	gl_Position = gl_ModelViewProjectionMatrix * vertexData;

}

-- Fragment

varying vec2 _texCoord;
varying vec2 vextexDataTramsformed;

uniform sampler2D texScreen;
uniform sampler2D texBloom;
uniform sampler2D texSSAO;
uniform sampler2D texVignette;
uniform sampler2D texWaterNoiseNM;

uniform sampler2D texBruit;
uniform sampler2D texBruit2;

uniform bool enable_pdc;
uniform bool enable_underwater;

uniform float noise_tile;
uniform float noise_factor;
uniform float time;

uniform float bloom_factor;

uniform bool enable_bloom;
uniform bool enable_vignette;
uniform bool enable_noise;
uniform bool enable_ssao;
uniform float randomCoeffNoise;
uniform float randomCoeffFlash;


vec4 SSAO(vec4 color){
	float ssaoFilter = texture(texSSAO, vextexDataTramsformed).r;
	//if(ssaoFilter > 0){
		//color.rgb = color.rgb * ssaoFilter;
	//}
	return color;
}

vec4 Bloom(vec4 color){

	return color + bloom_factor * texture(texBloom, _texCoord);
}

vec4 LevelOfGrey(vec4 colorIn)
{
	return vec4(colorIn.r * 0.299, colorIn.g * 0.587,colorIn.b * 0.114,colorIn.a);
}

vec4 NoiseEffect(vec4 colorIn)
{
	vec4 colorOut;
	
	vec4 colorNoise = texture(texBruit, _texCoord + vec2(randomCoeffNoise,randomCoeffNoise));
	
	colorOut = mix(colorNoise,vec4(1,1,1,1),randomCoeffFlash)/3.0f + 2.0*colorIn/3.0f;
	

	return colorOut;
}

vec4 VignetteEffect(vec4 colorIn)
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
	
	float time2 = time*0.0001;
	vec2 uvNormal0 = _texCoord*noise_tile;
	uvNormal0.s += time2;
	uvNormal0.t += time2;
	vec2 uvNormal1 = _texCoord*noise_tile;
	uvNormal1.s -= time2;
	uvNormal1.t += time2;
		
	vec3 normal0 = texture(texWaterNoiseNM, uvNormal0).rgb * 2.0 - 1.0;
//	vec3 normal1 = texture(texWaterNoiseNM, uvNormal1).rgb * 2.0 - 1.0;
//	vec3 normal = normalize(normal0+normal1);
	
	
	colorOut = texture(texScreen, _texCoord + noise_factor*normal0.st);
	
	colorOut = clamp(colorOut, vec4(0.0, 0.0, 0.0, 0.0),  vec4(1.0, 1.0, 1.0, 1.0));
	
	return colorOut;
}


void main(void){
	
	if(enable_underwater){
		gl_FragColor = UnderWater();
	}else{
		gl_FragColor = texture(texScreen, _texCoord);
	}

	if(enable_ssao){
		gl_FragColor = SSAO(gl_FragColor);
	}

	if(enable_bloom){
		gl_FragColor = Bloom(gl_FragColor);
	}

	if(enable_noise){
		gl_FragColor = LevelOfGrey(gl_FragColor);
		gl_FragColor = NoiseEffect(gl_FragColor);
	}
	
	if(enable_vignette){
		gl_FragColor = VignetteEffect(gl_FragColor);
	}
}
