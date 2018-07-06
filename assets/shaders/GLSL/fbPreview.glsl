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

uniform bool useScenePlanes = false;
layout(binding = TEXTURE_UNIT0) uniform sampler2D texDiffuse0;

void main()
{
    float n = dvd_ZPlanesCombined.x;
    float f = dvd_ZPlanesCombined.y * 0.5;
    if (useScenePlanes){
        n = dvd_ZPlanesCombined.z;
        f = dvd_ZPlanesCombined.w * 0.5;
    }

    float depth = texture(texDiffuse0, _texCoord).r;
    float linearDepth = (2 * n) / (f + n - (depth) * (f - n));
    _colorOut.rgb = vec3(linearDepth);
    _colorOut.a = 1.0;
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
uniform bool useScenePlanes = false;

void main()
{
    float n = dvd_ZPlanesCombined.x;
    float f = dvd_ZPlanesCombined.y * 0.5;
    if (useScenePlanes){
        n = dvd_ZPlanesCombined.z;
        f = dvd_ZPlanesCombined.w * 0.5;
    }

    float depth = texture(texDiffuse0, vec3(_texCoord, layer)).r;
    _colorOut.rgb = vec3((2 * n) / (f + n - (depth)* (f - n)));
}

--Fragment.Layered.LinearDepth.ESM

in vec2 _texCoord;
out vec4 _colorOut;

layout(binding = TEXTURE_UNIT0) uniform sampler2DArray texDiffuse0;
uniform int layer;

void main()
{
    float depth = texture(texDiffuse0, vec3(_texCoord, layer)).r;
    //depth = 1.0 - (log(depth) / DEPTH_EXP_WARP);
    
    _colorOut.rgb = vec3(depth);
}