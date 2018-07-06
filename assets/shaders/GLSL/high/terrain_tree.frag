uniform sampler2D texDiffuse0;
uniform sampler2D texDiffuse1;
uniform int textureCount;
uniform mat4 material;

void main (void)
{
	//vec4 cAmbient = gl_LightSource[0].ambient * material[0];
	vec4 cAmbient = material[0];
	vec4 cDiffuse = gl_LightSource[0].diffuse * material[1] * gl_Color;	
	
	if(textureCount > 0){
		vec4 base = texture2D(texDiffuse0, gl_TexCoord[0].st);
		if(base.a < 0.4) discard;
		cAmbient *= base;
		cDiffuse *= base;
	}
	if(textureCount > 1){
		vec4 base = texture2D(texDiffuse1, gl_TexCoord[0].st);
		cAmbient *= base;
		cDiffuse *= base;
	}
	
	gl_FragColor = cAmbient + cDiffuse;
	gl_FragColor.a = gl_Color.a;
}














