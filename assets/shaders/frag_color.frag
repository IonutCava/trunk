void applyTexture2D(in sampler2D texUnit, in int type, in int index, inout vec4 color);

uniform sampler2D TexUnit0;
uniform int TexturingType;

void main()
{
    vec4 color = gl_Color;
   
    applyTexture2D(TexUnit0, TexturingType, 0, color);
   
    gl_FragColor = color;
}