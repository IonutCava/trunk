--Vertex

void main(void)
{
    vec2 uv = vec2(0, 0);
    if ((gl_VertexID & 1) != 0) {
        uv.x = 1;
    }

    if ((gl_VertexID & 2) != 0) {
        uv.y = 1;
    }

    VAR._texCoord = uv * 2;
    gl_Position.xy = uv * 4 - 1;
    gl_Position.zw = vec2(0, 1);
}

-- Fragment

out vec4 _colorOut;

layout(binding = TEXTURE_UNIT0) uniform sampler2D texLeftEye;
layout(binding = TEXTURE_UNIT1) uniform sampler2D texRightEye;

uniform bool anaglyphEnabled;

void main(void){
    vec4 colorLeftEye = texture(texLeftEye, VAR._texCoord);
    if (anaglyphEnabled) {
        vec4 colorRightEye = texture(texRightEye, VAR._texCoord);
        _colorOut = vec4(colorLeftEye.r, colorRightEye.g, colorRightEye.b, 1.0);
    } else {
        _colorOut = colorLeftEye;
    }
}
