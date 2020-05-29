-- Vertex

#include "lightInput.cmn"

layout(location = 0) out float size;
layout(location = 1) out vec3 colour;
layout(location = 2) out flat vec2 texCoordOffset;

void main()
{
    Light light = dvd_LightSource[gl_VertexID];
    int lightType = int(light._options.x);

    texCoordOffset.x = lightType == LIGHT_SPOT ? 1 : 0;
    texCoordOffset.y = lightType == LIGHT_DIRECTIONAL ? 0 : 1;
                                
    colour = light._colour.rgb;
    size = light._positionWV.w * 0.5;

    if (lightType == LIGHT_DIRECTIONAL) {
        gl_Position = vec4(light._directionWV.xyz * light._positionWV.w * -1.0f, 1.0);
    } else {
        gl_Position = vec4(light._positionWV.xyz, 1.0);
    }
}

-- Geometry

layout(points) in;
layout(triangle_strip, max_vertices = 4) out;

layout(location = 0) in float size[];
layout(location = 1) in vec3 colour[];
layout(location = 2) in flat vec2 texCoordOffset[];

layout(location = 0) out vec3 lightColour;

void main()
{
    vec4 pos = gl_in[0].gl_Position;

    float size = size[0];
    lightColour = colour[0];
    
    // a: left-bottom 
    vec2 va = pos.xy + vec2(-0.5, -0.5) * size;
    gl_Position = dvd_ProjectionMatrix * vec4(va, pos.zw);
    _out._texCoord = vec2(0.0 + (texCoordOffset[0].x * 0.5),
                          (0.0 + (texCoordOffset[0].y * 0.5)));
    EmitVertex();

    // d: right-bottom
    vec2 vd = pos.xy + vec2(0.5, -0.5) * size;
    gl_Position = dvd_ProjectionMatrix * vec4(vd, pos.zw);
    _out._texCoord = vec2(0.5 + (texCoordOffset[0].x * 0.5),
                         (0.0 + (texCoordOffset[0].y * 0.5)));
    EmitVertex();

    // b: left-top
    vec2 vb = pos.xy + vec2(-0.5, 0.5) * size;
    gl_Position = dvd_ProjectionMatrix * vec4(vb, pos.zw);
    _out._texCoord = vec2(0.0 + (texCoordOffset[0].x * 0.5), 
                         (0.5 + (texCoordOffset[0].y * 0.5)));
    EmitVertex();
    
    // c: right-top
    vec2 vc = pos.xy + vec2(0.5, 0.5) * size;
    gl_Position = dvd_ProjectionMatrix * vec4(vc, pos.zw);
    _out._texCoord = vec2(0.5 + (texCoordOffset[0].x * 0.5),
                         (0.5 + (texCoordOffset[0].y * 0.5)));
    EmitVertex();

    EndPrimitive();
}

-- Fragment

layout(location = 0) in vec3 lightColour;

layout(binding = TEXTURE_UNIT0) uniform sampler2D texDiffuse0;

out vec4 colour;

void main()
{
    if (texture(texDiffuse0, VAR._texCoord).a < INV_Z_TEST_SIGMA) {
        discard;
    }
    
    colour = vec4(lightColour, 1.0);
    
}