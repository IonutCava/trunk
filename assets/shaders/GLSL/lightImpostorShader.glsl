-- Vertex


void main()
{
}


-- Geometry

layout(points) in;
layout(triangle_strip) out;
layout(max_vertices = 4) out;

#include "lightInput.cmn"

out flat int directionalIndex;
out flat int lightType;

void main()
{
    directionalIndex = LIGHT_DIRECTIONAL;

    Light light = dvd_LightSource[gl_PrimitiveIDIn];

    lightType = light._options.x;
    float size = light._positionWV.w;
    vec4 pos = vec4(light._positionWV.xyz, 1.0);

    // a: left-bottom 
    vec2 va = pos.xy + vec2(-0.5, -0.5) * size;
    gl_Position = dvd_ProjectionMatrix * vec4(va, pos.zw);
    g_out._texCoord = vec2(0.0, 0.0);
    EmitVertex();

    // b: left-top
    vec2 vb = pos.xy + vec2(-0.5, 0.5) * size;
    gl_Position = dvd_ProjectionMatrix * vec4(vb, pos.zw);
    g_out._texCoord = vec2(0.0, 1.0);
    EmitVertex();

    // d: right-bottom
    vec2 vd = pos.xy + vec2(0.5, -0.5) * size;
    gl_Position = dvd_ProjectionMatrix * vec4(vd, pos.zw);
    g_out._texCoord = vec2(1.0, 0.0);
    EmitVertex();

    // c: right-top
    vec2 vc = pos.xy + vec2(0.5, 0.5) * size;
    gl_Position = dvd_ProjectionMatrix * vec4(vc, pos.zw);
    g_out._texCoord = vec2(1.0, 1.0);
    EmitVertex();

    EndPrimitive();
}

-- Fragment

in flat int directionalIndex;
in flat int lightType;
out vec4 color;

void main()
{
    if (lightType == directionalIndex) {
        discard;
    }

    color = vec4(1.0, 0.0, 0.0, 1.0);
    
}