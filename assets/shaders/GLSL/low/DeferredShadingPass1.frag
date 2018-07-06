varying vec3 normals;
varying vec3 position;
varying vec4 texCoord[2];

uniform sampler2D    texDiffuse0;

void main( void ){

	gl_FragData[0] = texture2D(texDiffuse0,texCoord[0].st);
	gl_FragData[0].a = 1;
    gl_FragData[1] = vec4(position,1);
    gl_FragData[2] = vec4(normals.xyz,0);
}