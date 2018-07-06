-- Vertex
out vec2 _texCoord;

void main(void)
{
    vec2 uv = vec2(0,0);
    if((gl_VertexID & 1) != 0)uv.x = 1;
    if((gl_VertexID & 2) != 0)uv.y = 1;

    _texCoord = uv * 2;
    gl_Position.xy = uv * 4 - 1;
    gl_Position.zw = vec2(0,1);
}

-- Fragment

in vec2 _texCoord;
out vec4 _colorOut;

layout(binding = TEXTURE_UNIT0) uniform sampler2D texDiffuse0;

void main()
{
    _colorOut = texture(texDiffuse0, _texCoord);
    _colorOut.a = 1.0;
}

-- Fragment.LinearDepth

in vec2 _texCoord;
out vec4 _colorOut;

layout(binding = TEXTURE_UNIT0) uniform sampler2D texDiffuse0;

void main()
{
#if defined(USE_SCENE_ZPLANES)
	float n = dvd_ZPlanesCombined.z;
	float f = dvd_ZPlanesCombined.w * 0.5;
#else
	float n = dvd_ZPlanesCombined.x;
    float f = dvd_ZPlanesCombined.y * 0.5;
#endif

    float depth = texture(texDiffuse0, _texCoord).r;
    float linearDepth = (2 * n) / (f + n - (depth) * (f - n));
    _colorOut = vec4(vec3(linearDepth), 1.0);
}

-- Fragment.Layered

in vec2 _texCoord;
out vec4 _colorOut;

layout(binding = TEXTURE_UNIT0) uniform sampler2DArray texDiffuse0;
uniform int layer;

void main()
{
    _colorOut = texture(texDiffuse0, vec3(_texCoord, layer));
}

-- Fragment.Layered.LinearDepth

in vec2 _texCoord;
out vec4 _colorOut;


layout(binding = TEXTURE_UNIT0) uniform sampler2DArray texDiffuse0;
uniform int layer;

void main()
{
#if defined(USE_SCENE_ZPLANES)
	float n = dvd_ZPlanesCombined.z;
	float f = dvd_ZPlanesCombined.w * 0.5;
#else
	float n = dvd_ZPlanesCombined.x;
    float f = dvd_ZPlanesCombined.y * 0.5;
#endif

    float depth = texture(texDiffuse0, vec3(_texCoord, layer)).r;
	float linearDepth = (2 * n) / (f + n - (depth) * (f - n));
    _colorOut = vec4(vec3(linearDepth), 1.0);
}

--Fragment.Layered.LinearDepth.ESM

in vec2 _texCoord;
out vec4 _colorOut;

layout(binding = TEXTURE_UNIT0) uniform sampler2DArray texDiffuse0;
uniform int layer;

#if !defined(USE_SCENE_ZPLANES)
uniform vec2 dvd_zPlanes;
#endif

void main()
{
#if defined(USE_SCENE_ZPLANES)
	float n = dvd_ZPlanesCombined.z;
	float f = dvd_ZPlanesCombined.w * 0.5;
#else
	float n = dvd_zPlanes.x;
    float f = dvd_zPlanes.y * 0.5;
#endif

    float depth = texture(texDiffuse0, vec3(_texCoord, layer)).r;
    //depth = 1.0 - (log(depth) / DEPTH_EXP_WARP);
    
	float linearDepth = (2 * n) / (f + n - (depth) * (f - n));
    _colorOut = vec4(vec3(linearDepth), 1.0);
}