-- Vertex

uniform mat4 ProjMtx;
in vec2 Position;

out vec2 Frag_UV;
out vec4 Frag_Color;

void main()
{
    Frag_UV = inTexCoordData;
    Frag_Color = inColourData;
    gl_Position = ProjMtx * vec4(Position.xy,0,1);
};

-- Fragment

layout(binding = TEXTURE_UNIT0) uniform sampler2D Texture;\n

in vec2 Frag_UV;
in vec4 Frag_Color;
out vec4 Out_Color;

void main()
{
	Out_Color = Frag_Color * texture( Texture, Frag_UV.st);
};