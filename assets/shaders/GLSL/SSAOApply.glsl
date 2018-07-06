--Vertex

void main(void)
{
    vec2 uv = vec2(0, 0);
    if ((gl_VertexID & 1) != 0)uv.x = 1;
    if ((gl_VertexID & 2) != 0)uv.y = 1;

    VAR._texCoord = uv * 2;
    gl_Position.xy = uv * 4 - 1;
    gl_Position.zw = vec2(0, 1);
}

--Fragment

layout(binding = TEXTURE_UNIT0) uniform sampler2D texScreen;
layout(binding = TEXTURE_UNIT1) uniform sampler2D texSSAO;

out vec4 _colorOut;

void main () {
    float ssaoFilter = texture(texSSAO, VAR._texCoord).r;
    _colorOut = texture(texScreen, VAR._texCoord);

    if (ssaoFilter > 0) {
        _colorOut.rgb = _colorOut.rgb * ssaoFilter;
    }
}