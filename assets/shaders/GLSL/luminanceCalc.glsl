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

out vec4 _colorOut;

layout(binding = TEXTURE_UNIT0) uniform sampler2D texScreen;
layout(binding = TEXTURE_UNIT1) uniform sampler2D texPrevExposure;

void main() {
    vec3 screenColor = texture(texScreen, VAR._texCoord).rgb;
    float PreviousExposure = texture(texPrevExposure, VAR._texCoord).r;
    float TargetExposure = log(0.5 / clamp(dot(screenColor, vec3(0.299, 0.587, 0.114)), 0.3, 0.7));

    _colorOut.r = mix(PreviousExposure, TargetExposure, 0.1);
}