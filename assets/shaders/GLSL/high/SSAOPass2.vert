varying vec4 texCoord[2];

void main(void){
	gl_Position = ftransform();
	gl_Position = sign( gl_Position );
 
	// Texture coordinate for screen aligned (in correct range):
	texCoord[0].st = (vec2( gl_Position.x, - gl_Position.y ) + vec2( 1.0 ) ) * 0.5;
}