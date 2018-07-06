uniform sampler2D texDiffuse0;
uniform sampler2D texDiffuse1;
uniform int textureCount;

void main (void)
{
	vec4 cBase = vec4(1.0f,1.0f,1.0f,1.0f);
	vec4 outColor = vec4(1.0f,1.0f,1.0f,1.0f);

	vec4 cAmbient = gl_LightSource[0].ambient * gl_FrontMaterial.ambient;
	vec4 cDiffuse = gl_LightSource[0].diffuse * gl_FrontMaterial.diffuse * gl_Color;	
		
	if(textureCount > 0)
	{
		cBase = texture2D(texDiffuse0, gl_TexCoord[0].st);
		if(textureCount == 2) cBase *= texture2D(texDiffuse1, gl_TexCoord[0].st);
		if(cBase.a < 0.4) discard;
		outColor = cAmbient * cBase + cDiffuse * cBase; 
	}
	else
	{
		outColor = cAmbient + cDiffuse;
	}


	gl_FragColor = outColor ;

}
















