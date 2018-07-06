-- Vertex

#include "lightInput.cmn"

out float size;
out vec3 color;
out flat int lightType;
out flat int skipLightType;
out flat int texCoordOffset;

void main()
{
    Light light = dvd_LightSource[gl_VertexID];
    lightType = int(light._options.x);
    skipLightType = LIGHT_DIRECTIONAL;

    texCoordOffset = lightType == LIGHT_OMNIDIRECTIONAL ? 0 : 1;

    color = light._color.rgb;
    size = /*light._positionWV.w **/ 2.0;
    gl_Position = vec4(light._positionWV.xyz, 1.0);
}

-- Geometry

layout(points) in;
layout(triangle_strip, max_vertices = 4) out;

in float size[];
in vec3 color[];
in flat int lightType[];
in flat int skipLightType[];
in flat int texCoordOffset[];

out vec3 lightColor;
out flat int skipLight;

void main()
{
    vec4 pos = gl_in[0].gl_Position;

    skipLight = lightType[0] == skipLightType[0] ? 1 : 0;
    float size = size[0];
    lightColor = color[0];
    
    // a: left-bottom 
    vec2 va = pos.xy + vec2(-0.5, -0.5) * size;
    gl_Position = dvd_ProjectionMatrix * vec4(va, pos.zw);
    g_out._texCoord = vec2(0.0 + (texCoordOffset[0] * 0.5), 0.0);
    EmitVertex();

    // d: right-bottom
    vec2 vd = pos.xy + vec2(0.5, -0.5) * size;
    gl_Position = dvd_ProjectionMatrix * vec4(vd, pos.zw);
    g_out._texCoord = vec2(0.5 + (texCoordOffset[0] * 0.5), 0.0);
    EmitVertex();

    // b: left-top
    vec2 vb = pos.xy + vec2(-0.5, 0.5) * size;
    gl_Position = dvd_ProjectionMatrix * vec4(vb, pos.zw);
    g_out._texCoord = vec2(0.0 + (texCoordOffset[0] * 0.5), 1.0);
    EmitVertex();
    
    // c: right-top
    vec2 vc = pos.xy + vec2(0.5, 0.5) * size;
    gl_Position = dvd_ProjectionMatrix * vec4(vc, pos.zw);
    g_out._texCoord = vec2(0.5 + (texCoordOffset[0] * 0.5), 1.0);
    EmitVertex();

    EndPrimitive();
}

-- Fragment

in vec3 lightColor;
in flat int skipLight;

layout(binding = TEXTURE_UNIT0) uniform sampler2D texDiffuse0;

out vec4 color;

void main()
{
    if (skipLight == 1) {
        discard;
    }

    if (texture(texDiffuse0, VAR._texCoord).a < 0.95) {
        discard;
    }
    
    color = vec4(lightColor, 1.0);
    
}