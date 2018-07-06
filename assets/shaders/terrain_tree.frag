uniform sampler2D texDiffuse0;
uniform sampler2D texDiffuse1;
uniform int textureCount;
uniform vec4      diffuse;
uniform vec4      ambient;
uniform vec4      specular;

void main (void)
{

	vec4 base = diffuse;
	vec4 outColor = vec4(1.0f,1.0f,1.0f,1.0f);

	vec4 cAmbient = gl_LightSource[0].ambient * ambient;
	vec4 cDiffuse = gl_LightSource[0].diffuse * diffuse * gl_Color;	
	vec4 cSpecular = gl_LightSource[0].specular * specular ;	
	
	if(textureCount > 0){
		base = texture2D(texDiffuse0, gl_TexCoord[0].st);
		if(textureCount== 2) base *= texture2D(texDiffuse1, gl_TexCoord[0].st);
		if(base.a < 0.4) discard;
	}
	
	outColor = cAmbient * base + (cDiffuse * base + cSpecular);
	if(outColor.a < 0.2) discard;
	
	
	gl_FragColor = outColor;
}














