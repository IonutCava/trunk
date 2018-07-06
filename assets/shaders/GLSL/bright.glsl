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

in vec2  _texCoord;
out vec4 _colorOut;

layout(binding = TEXTURE_UNIT0) uniform sampler2D texScreen;
uniform sampler2D texExposure;
uniform sampler2D texPrevExposure;
uniform int  exposureMipLevel;

uniform bool toneMap = false;
uniform bool luminancePass = false;

uniform float whitePoint = 0.9;

vec3 Uncharted2Tonemap(vec3 x)
{
	float A = 0.15;
	float B = 0.50;
	float C = 0.10;
	float D = 0.20;
	float E = 0.02;
	float F = 0.30;

	return ((x*(A*x+C*B)+D*E)/(x*(A*x+B)+D*F))-E/F;
}

void main(){	
	vec3 value = texture(texScreen, _texCoord).rgb;

	if(luminancePass){
		float PreviousExposure = texture(texPrevExposure, vec2(0.5, 0.5)).r;
		float TargetExposure = 0.5 / clamp(dot(value, vec3(0.299, 0.587, 0.114)), 0.1, 0.9);
		_colorOut.r = PreviousExposure + (TargetExposure - PreviousExposure) * 0.01;
	}else{
		if(toneMap) {
			float exposure = textureLod(texExposure, vec2(0.5,0.5), exposureMipLevel).r;
			_colorOut = vec4(Uncharted2Tonemap(value * exposure) / Uncharted2Tonemap(vec3(whitePoint)) , 1.0);
		}else{
			_colorOut = clamp(sign(((value.r + value.g + value.b)/3.0) - whitePoint) * vec4(1.0), 0.0, 1.0);
		}
	}
}
