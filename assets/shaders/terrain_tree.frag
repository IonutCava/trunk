uniform sampler2D texDiffuse;
uniform int texture;

void main (void)
{

	vec4 cBase = texture2D(texDiffuse, gl_TexCoord[0].st);
	if(texture != 1) cBase = gl_Color;

	vec4 cAmbient = gl_LightSource[0].ambient * gl_FrontMaterial.ambient;
	vec4 cDiffuse = gl_LightSource[0].diffuse * gl_FrontMaterial.diffuse * gl_Color;	
	if(texture == 1) if(cBase.a < 0.4) discard;
	
	gl_FragColor = cAmbient * cBase + cDiffuse * cBase;
}














