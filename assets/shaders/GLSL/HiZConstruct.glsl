//http://rastergrid.com/blog/2010/10/hierarchical-z-map-based-occlusion-culling/

--Vertex

void main(void)
{
}

--Geometry

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


--Fragment

layout(binding = TEXTURE_UNIT0) uniform sampler2D LastMip;
uniform ivec2 LastMipSize;

in vec2 _texCoord;

void main(void)
{
    vec4 texels;
    texels.x = texture(LastMip, _texCoord).x;
    texels.y = textureOffset(LastMip, _texCoord, ivec2(-1, 0)).x;
    texels.z = textureOffset(LastMip, _texCoord, ivec2(-1, -1)).x;
    texels.w = textureOffset(LastMip, _texCoord, ivec2(0, -1)).x;

    float maxZ = max(max(texels.x, texels.y), max(texels.z, texels.w));

    vec3 extra;
    // if we are reducing an odd-width texture then the edge fragments have to fetch additional texels
    if (((LastMipSize.x & 1) != 0) && (int(gl_FragCoord.x) == LastMipSize.x - 3)) {
        // if both edges are odd, fetch the top-left corner texel
        if (((LastMipSize.y & 1) != 0) && (int(gl_FragCoord.y) == LastMipSize.y - 3)) {
            extra.z = textureOffset(LastMip, _texCoord, ivec2(1, 1)).x;
            maxZ = max(maxZ, extra.z);
        }
        extra.x = textureOffset(LastMip, _texCoord, ivec2(1, 0)).x;
        extra.y = textureOffset(LastMip, _texCoord, ivec2(1, -1)).x;
        maxZ = max(maxZ, max(extra.x, extra.y));
    }
    else
        // if we are reducing an odd-height texture then the edge fragments have to fetch additional texels
    if (((LastMipSize.y & 1) != 0) && (int(gl_FragCoord.y) == LastMipSize.y - 3)) {
        extra.x = textureOffset(LastMip, _texCoord, ivec2(0, 1)).x;
        extra.y = textureOffset(LastMip, _texCoord, ivec2(-1, 1)).x;
        maxZ = max(maxZ, max(extra.x, extra.y));
    }

    gl_FragDepth = maxZ;
}