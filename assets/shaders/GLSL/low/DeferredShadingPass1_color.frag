varying vec3         normals;
varying vec3         position;
uniform mat4         material;
uniform int       mode;
#define MODE_DEPTH -1

void main( void )
{
	if(mode == MODE_DEPTH)
		gl_FragData[0] = vec4(1.0,1.0,1.0,1.0);
	else
		gl_FragData[0] = material[1]; //diffuse

    gl_FragData[1] = vec4(position,1);
    gl_FragData[2] = vec4(normals.xyz,0);
}