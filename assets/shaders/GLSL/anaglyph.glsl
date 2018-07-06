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

uniform sampler2D texLeftEye;
uniform sampler2D texRightEye;

void main(void){
    vec4 colorLeftEye = texture(texLeftEye, VAR._texCoord);
    vec4 colorRightEye = texture(texRightEye, VAR._texCoord);
    _colorOut = vec4(colorLeftEye.r, colorRightEye.g, colorRightEye.b, 1.0);
}
