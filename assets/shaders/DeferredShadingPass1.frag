varying vec3         normals;
varying vec3         position;
uniform sampler2D    texDiffuse0;

void main( void )
{
   gl_FragData[0] = texture2D(texDiffuse0,gl_TexCoord[0].st);
   if(gl_FragData[0].a < 0.4) discard;
   gl_FragData[0].a = 1;
   gl_FragData[1] = vec4(position,1);
   gl_FragData[2] = vec4(normals.xyz,0);
}