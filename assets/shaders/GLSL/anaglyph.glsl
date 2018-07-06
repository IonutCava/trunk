-- Vertex

varying vec4 texCoord[2];

void main(void){
	texCoord[0] = gl_MultiTexCoord0;
	gl_Position = ftransform();
}


-- Fragment

varying vec4 texCoord[2];

uniform sampler2D texLeftEye;
uniform sampler2D texRightEye;

void main(void){
	vec4 colorLeftEye = texture2D(texLeftEye, texCoord[0].st);
	vec4 colorRightEye = texture2D(texRightEye, texCoord[0].st);
	gl_FragColor = vec4(colorLeftEye.r, colorRightEye.g, colorRightEye.b, 1.0);
}
