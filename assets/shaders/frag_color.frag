void applyTexture2D(in sampler2D texUnit, in int type, in int index, inout vec4 color);

uniform sampler2D texDiffuse0;
uniform int TexturingType;

void main()
{
    vec4 color = gl_Color;
   
    applyTexture2D(texDiffuse0, TexturingType, 0, color);
   
    gl_FragColor = color;
}