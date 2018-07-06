varying vec3         normals;
varying vec4         position;
uniform sampler2D    texDiffuse0;
uniform sampler2D    texBump;
uniform int       mode;
#define MODE_DEPTH -1

void main( void )
{
	if(mode == MODE_DEPTH)
		gl_FragData[0] = vec4(1.0,1.0,1.0,1.0);
	else
	   gl_FragData[0] = texture2D(texDiffuse0,gl_TexCoord[0].st);

   gl_FragData[0].a = 1;
   gl_FragData[1] = vec4(position.xyz,1);
   gl_FragData[2] = vec4(normals.xyz,1);
}