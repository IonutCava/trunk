-- Vertex

varying vec4 texCoord[2];

void main(){

  texCoord[0] = gl_MultiTexCoord0;
  gl_Position = ftransform();
} 

-- Fragment

varying vec4 texCoord[2];
uniform sampler2D texScreen;
uniform float threshold;

void main(){	
	vec4 value = texture2D(texScreen, texCoord[0].st);
		
	if( (value.r + value.g + value.b)/3.0 > threshold )
		gl_FragColor = vec4(1.0, 1.0, 1.0, 1.0);
	else
		gl_FragColor = vec4(0.0, 0.0, 0.0, 1.0);

}
