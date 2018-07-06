uniform sampler2D texDiffuse0;
uniform sampler2D texDiffuse1;
uniform int		  textureCount;
uniform mat4      material;

void main (void)
{
	vec4 cAmbient = gl_LightSource[0].ambient * material[0];
	vec4 cDiffuse = gl_LightSource[0].diffuse * material[1] * gl_Color;	
	vec4 cSpecular = gl_LightSource[0].specular * material[2] ;	
	
	if(textureCount > 0){
		vec4 base = texture2D(texDiffuse0, gl_TexCoord[0].st);
		if(textureCount == 2) base += texture2D(texDiffuse1, gl_TexCoord[0].st);
		cAmbient *= base;
		cDiffuse *= base;
	}
	vec4 colorOut = cAmbient + (cDiffuse + cSpecular);
	if(colorOut.a < 0.2) discard;

	gl_FragColor = colorOut ;

}
















