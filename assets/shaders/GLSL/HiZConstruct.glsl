//http://rastergrid.com/blog/2010/10/hierarchical-z-map-based-occlusion-culling/

-- Vertex

void main(void)
{
    vec2 uv = vec2(0,0);
    if((gl_VertexID & 1) != 0)uv.x = 1;
    if((gl_VertexID & 2) != 0)uv.y = 1;

    VAR._texCoord = uv * 2;
    gl_Position.xy = uv * 4 - 1;
    gl_Position.zw = vec2(0,1);
}


--Fragment

layout(binding = TEXTURE_DEPTH_MAP) uniform sampler2D LastMip;

void main(void)
{
    ivec2 LastMipSize = ivec2(dvd_ViewPort.z, dvd_ViewPort.w);

    // texture should not be linear, so this wouldn't work
    //vec4 texels = textureGather(LastMip, VAR._texCoord, 0);
    vec4 texels;
    texels.x = texture(LastMip, VAR._texCoord).x;
    texels.y = textureOffset(LastMip, VAR._texCoord, ivec2(-1,  0)).x;
    texels.z = textureOffset(LastMip, VAR._texCoord, ivec2(-1, -1)).x;
    texels.w = textureOffset(LastMip, VAR._texCoord, ivec2( 0, -1)).x;

    float maxZ = max(max(texels.x, texels.y), max(texels.z, texels.w));

    vec3 extra;
    // if we are reducing an odd-width texture then the edge fragments have to fetch additional texels
    if (((LastMipSize.x & 1) != 0) && (int(gl_FragCoord.x) == LastMipSize.x - 3)) {
        // if both edges are odd, fetch the top-left corner texel
        if (((LastMipSize.y & 1) != 0) && (int(gl_FragCoord.y) == LastMipSize.y - 3)) {
            extra.z = textureOffset(LastMip, VAR._texCoord, ivec2(1, 1)).x;
            maxZ = max(maxZ, extra.z);
        }
        extra.x = textureOffset(LastMip, VAR._texCoord, ivec2(1,  0)).x;
        extra.y = textureOffset(LastMip, VAR._texCoord, ivec2(1, -1)).x;
        maxZ = max(maxZ, max(extra.x, extra.y));
    // if we are reducing an odd-height texture then the edge fragments have to fetch additional texels
    } else if (((LastMipSize.y & 1) != 0) && (int(gl_FragCoord.y) == LastMipSize.y - 3)) {
        extra.x = textureOffset(LastMip, VAR._texCoord, ivec2( 0, 1)).x;
        extra.y = textureOffset(LastMip, VAR._texCoord, ivec2(-1, 1)).x;
        maxZ = max(maxZ, max(extra.x, extra.y));
    }

    gl_FragDepth = maxZ;
}