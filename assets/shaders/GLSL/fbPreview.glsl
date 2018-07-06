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

uniform sampler2D tex;

void main()
{
    _colorOut = texture(tex, _texCoord);
    _colorOut.a = 1.0;
}

-- Fragment.LinearDepth

in vec2 _texCoord;
out vec4 _colorOut;

uniform bool useScenePlanes = false;
uniform sampler2D tex;

void main()
{
    float n = dvd_zPlanes.x;
    float f = dvd_zPlanes.y * 0.5;
    if (useScenePlanes){
        n = dvd_sceneZPlanes.x;
        f = dvd_sceneZPlanes.y * 0.5;
    }

    float depth = texture(tex, _texCoord).r;
    float linearDepth = (2 * n) / (f + n - (depth) * (f - n));
    _colorOut.rgb = vec3(linearDepth);
    _colorOut.a = 1.0;
}

-- Fragment.Layered

in vec2 _texCoord;
out vec4 _colorOut;

uniform sampler2DArray tex;
uniform int layer;

void main()
{
    _colorOut = texture(tex, vec3(_texCoord, layer));
}

-- Fragment.Layered.LinearDepth

in vec2 _texCoord;
out vec4 _colorOut;

uniform sampler2DArray tex;
uniform int layer;
uniform bool useScenePlanes = false;

void main()
{
    float n = dvd_zPlanes.x;
    float f = dvd_zPlanes.y * 0.5;
    if (useScenePlanes){
        n = dvd_sceneZPlanes.x;
        f = dvd_sceneZPlanes.y * 0.5;
    }

    float depth = texture(tex, vec3(_texCoord, layer)).r;
    _colorOut.rgb = vec3((2 * n) / (f + n - (depth)* (f - n)));
}

--Fragment.Layered.LinearDepth.ESM

in vec2 _texCoord;
out vec4 _colorOut;

uniform sampler2DArray tex;
uniform int layer;

void main()
{
    float depth = texture(tex, vec3(_texCoord, layer)).r;
    //depth = 1.0 - (log(depth) / DEPTH_EXP_WARP);
    
    _colorOut.rgb = vec3(depth);
}