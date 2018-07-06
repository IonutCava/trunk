-- Vertex 

void main(void)
{
}

-- Geometry

layout(points) in;
layout(triangle_strip, max_vertices = 4) out;

out vec2 _texCoord;

void main() {
    gl_Position = vec4(1.0, 1.0, 0.0, 1.0);
    _texCoord = vec2(1.0, 1.0);
    EmitVertex();

    gl_Position = vec4(-1.0, 1.0, 0.0, 1.0);
    _texCoord = vec2(0.0, 1.0);
    EmitVertex();

    gl_Position = vec4(1.0, -1.0, 0.0, 1.0);
    _texCoord = vec2(1.0, 0.0);
    EmitVertex();

    gl_Position = vec4(-1.0, -1.0, 0.0, 1.0);
    _texCoord = vec2(0.0, 0.0);
    EmitVertex();

    EndPrimitive();
}


-- Fragment

in  vec2 _texCoord;
out vec4 _colorOut;

layout(binding = TEXTURE_UNIT0) uniform sampler2D texScreen;
uniform sampler2D texVignette;
uniform sampler2D texWaterNoiseNM;

uniform sampler2D texNoise;

uniform float _noiseTile;
uniform float _noiseFactor;
uniform float dvd_time;

uniform sampler2D texBloom;
uniform float bloomFactor;

uniform sampler2D texSSAO;

uniform float randomCoeffNoise;
uniform float randomCoeffFlash;

subroutine vec4 VignetteRoutineType(in vec4 colorIn);
subroutine vec4 NoiseRoutineType(in vec4 colorIn);
subroutine vec4 BloomRoutineType(in vec4 colorIn);
subroutine vec4 SSAORoutineType(in vec4 colorIn);
subroutine vec4 ScreenRoutineType();
subroutine void OutputRoutineType();

layout(location = 0) subroutine uniform VignetteRoutineType VignetteRoutine;
layout(location = 1) subroutine uniform NoiseRoutineType NoiseRoutine;
layout(location = 2) subroutine uniform BloomRoutineType BloomRoutine;
layout(location = 3) subroutine uniform SSAORoutineType SSAORoutine;
layout(location = 4) subroutine uniform ScreenRoutineType ScreenRoutine;
layout(location = 5) subroutine uniform OutputRoutineType OutputRoutine;

vec4 LevelOfGrey(in vec4 colorIn) {
	return vec4(colorIn.r * 0.299, colorIn.g * 0.587, colorIn.b * 0.114, colorIn.a);
}


subroutine(VignetteRoutineType)
vec4 Vignette(in vec4 colorIn){
	vec4 colorOut = colorIn - (vec4(1,1,1,2) - texture(texVignette, _texCoord));
	return vec4(clamp(colorOut.rgb,0.0,1.0), colorOut.a);
}


subroutine(NoiseRoutineType)
vec4 Noise(in vec4 colorIn){
	return mix(texture(texNoise, _texCoord + vec2(randomCoeffNoise, randomCoeffNoise)), 
		       vec4(1.0), randomCoeffFlash) / 3.0 + 2.0 * LevelOfGrey(colorIn) / 3.0;
}


subroutine(BloomRoutineType)
vec4 Bloom(in vec4 colorIn){
	return colorIn + bloomFactor * texture(texBloom, _texCoord);
}


subroutine(SSAORoutineType)
vec4 SSAO(in vec4 colorIn){
	float ssaoFilter = texture(texSSAO, _texCoord).r;
	if(ssaoFilter > 0){
		colorIn.rgb = colorIn.rgb * ssaoFilter;
	}
	return colorIn;
}

subroutine(ScreenRoutineType)
vec4 screenUnderwater(){
	float time2 = dvd_time*0.0001;
	vec2 noiseUV = _texCoord * _noiseTile;
    vec2 uvNormal0 = noiseUV + time2;
	vec2 uvNormal1 = noiseUV;
	uvNormal1.s -= time2;
	uvNormal1.t += time2;
		
	vec3 normal0 = texture(texWaterNoiseNM, uvNormal0).rgb;
	vec3 normal1 = texture(texWaterNoiseNM, uvNormal1).rgb;
    vec3 normal = normalize(2.0 * ((normal0 + normal1) * 0.5) - 1.0);
	
    return clamp(texture(texScreen, _texCoord + _noiseFactor * normal.st), vec4(0.0), vec4(1.0));
}

subroutine(ScreenRoutineType)
vec4 screenNormal(){
	return texture(texScreen, _texCoord);
}

subroutine(SSAORoutineType, BloomRoutineType, NoiseRoutineType, VignetteRoutineType)
vec4 ColorPassThrough(in vec4 colorIn){
	return colorIn;
}

subroutine(OutputRoutineType)
void outputScreen(){
    _colorOut = VignetteRoutine(NoiseRoutine(BloomRoutine(SSAORoutine(ScreenRoutine()))));
}

subroutine(OutputRoutineType)
void outputDepth(){
	float near = dvd_sceneZPlanes.x;
	float far = dvd_sceneZPlanes.y;
    _colorOut = vec4(vec3((2 * near) / (far + near - texture(texScreen, _texCoord).r * (far - near))), 1.0);
}

void main(void){
    OutputRoutine();
}
