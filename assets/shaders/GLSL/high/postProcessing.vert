varying vec3 vPixToLight;		// Vecteur du pixel courant à la lumière
varying vec3 vPixToEye;			// Vecteur du pixel courant à l'oeil
varying vec4 texCoord[2];

void main(void)
{
	texCoord[0] = gl_MultiTexCoord0;
	texCoord[1].st = (vec2( gl_Position.x, - gl_Position.y ) + vec2( 1.0 ) ) * 0.5;
	gl_Position = ftransform();

}
