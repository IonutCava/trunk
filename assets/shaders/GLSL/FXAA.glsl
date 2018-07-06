-- Vertex

void main(void)
{
}

-- Geometry

layout(points) in;
layout(triangle_strip, max_vertices = 4) out;

out vec2 _texCoord;
out vec4 _posPos;

uniform float dvd_fxaaSubpixShift = 1.0 / 4.0;

void main() {
    vec2 offset = dvd_invScreenDimensions() * (0.5 + dvd_fxaaSubpixShift);

    gl_Position = vec4(1.0, 1.0, 0.0, 1.0);
    _texCoord = vec2(1.0, 1.0);
    _posPos = vec4(_texCoord, _texCoord - offset);
    EmitVertex();

    gl_Position = vec4(-1.0, 1.0, 0.0, 1.0);
    _texCoord = vec2(0.0, 1.0);
    _posPos = vec4(_texCoord, _texCoord - offset);
    EmitVertex();

    gl_Position = vec4(1.0, -1.0, 0.0, 1.0);
    _texCoord = vec2(1.0, 0.0);
    _posPos = vec4(_texCoord, _texCoord - offset);
    EmitVertex();

    gl_Position = vec4(-1.0, -1.0, 0.0, 1.0);
    _texCoord = vec2(0.0, 0.0);
    _posPos = vec4(_texCoord, _texCoord - offset);
    EmitVertex();

    EndPrimitive();
}

-- Fragment

#define FxaaTexLod0(t, p) textureLod(t, p, 0.0)
#define FxaaTexOff(t, p, o, r) textureLodOffset(t, p, 0.0, o)

in  vec4 _posPos;
out vec4 _colorOut;

layout(binding = TEXTURE_UNIT0) uniform sampler2D texScreen;
uniform float dvd_fxaaSpanMax = 8.0;
uniform float dvd_fxaaReduceMul = 1.0 / 8.0;
uniform float dvd_fxaaReduceMin = 1.0 / 128.0;

void main(){

/*---------------------------------------------------------*/
    const vec3 luma = vec3(0.299, 0.587, 0.114);
    float lumaNW = dot(FxaaTexLod0(texScreen, _posPos.zw).xyz ,luma);
    float lumaNE = dot(FxaaTexOff(texScreen, _posPos.zw, ivec2(1,0), dvd_invScreenDimensions()).xyz ,luma);
    float lumaSW = dot(FxaaTexOff(texScreen, _posPos.zw, ivec2(0,1), dvd_invScreenDimensions()).xyz ,luma);
    float lumaSE = dot(FxaaTexOff(texScreen, _posPos.zw, ivec2(1,1), dvd_invScreenDimensions()).xyz ,luma);
    float lumaM  = dot(FxaaTexLod0(texScreen, _posPos.xy).xyz, luma);
/*---------------------------------------------------------*/
    float lumaMin = min(lumaM, min(min(lumaNW, lumaNE), min(lumaSW, lumaSE)));
    float lumaMax = max(lumaM, max(max(lumaNW, lumaNE), max(lumaSW, lumaSE)));
/*---------------------------------------------------------*/
    vec2 dir;
    dir.x = -((lumaNW + lumaNE) - (lumaSW + lumaSE));
    dir.y =  ((lumaNW + lumaSW) - (lumaNE + lumaSE));
/*---------------------------------------------------------*/
    float dirReduce = max((lumaNW + lumaNE + lumaSW + lumaSE) * (0.25 * dvd_fxaaReduceMul), dvd_fxaaReduceMin);
    float rcpDirMin = 1.0/(min(abs(dir.x), abs(dir.y)) + dirReduce);
    dir = min(vec2( dvd_fxaaSpanMax,  dvd_fxaaSpanMax), 
              max(vec2(-dvd_fxaaSpanMax, -dvd_fxaaSpanMax), 
                  dir * rcpDirMin)) * dvd_invScreenDimensions();
/*--------------------------------------------------------*/
    vec3 rgbA = (1.0/2.0) * ( FxaaTexLod0(texScreen, _posPos.xy + dir * (1.0/3.0 - 0.5)).xyz +
                              FxaaTexLod0(texScreen, _posPos.xy + dir * (2.0/3.0 - 0.5)).xyz);
    vec3 rgbB = rgbA * (1.0/2.0) + (1.0/4.0) * ( FxaaTexLod0(texScreen, _posPos.xy + dir * (0.0/3.0 - 0.5)).xyz +
                                                 FxaaTexLod0(texScreen, _posPos.xy + dir * (3.0/3.0 - 0.5)).xyz);
    float lumaB = dot(rgbB, luma);

    
    if((lumaB < lumaMin) || (lumaB > lumaMax)) _colorOut.rgb = rgbA;
    else _colorOut.rgb = rgbB; 

    _colorOut.a = 1.0;
}