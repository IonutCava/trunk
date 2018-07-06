varying vec4 texCoord[2];

uniform sampler2D texDiffuse0;
uniform sampler2D texDiffuse1;
uniform int		  textureCount;
uniform mat4      material;


void main (void){
	vec4 color = vec4(1.0, 0.0, 0.0, 1.0);

	vec4 cAmbient = gl_LightSource[0].ambient * material[0];
	vec4 cDiffuse = gl_LightSource[0].diffuse * material[1];	
	vec4 cSpecular = gl_LightSource[0].specular * material[2] ;	
		
	if(textureCount > 0){
		vec4 base = texture2D(texDiffuse0, texCoord[0].st);
		if(textureCount == 2) base += texture2D(texDiffuse1, texCoord[0].st);
		cAmbient *= base;
		cDiffuse *= base;
	}
	color = cAmbient + (cDiffuse + cSpecular);
	if(color.a < 0.2) discard;
	
	gl_FragColor = color;

}
















