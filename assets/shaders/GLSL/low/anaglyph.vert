varying vec4 texCoord[2];

void main(void)
{
	texCoord[0] = gl_MultiTexCoord0;
	gl_Position = ftransform();
}
