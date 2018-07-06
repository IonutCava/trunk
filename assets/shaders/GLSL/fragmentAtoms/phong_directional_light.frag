

void phong_directionalLight(in int light, 
							in vec4 specularIn, 
							in float iSpecular,
							in float NdotL,
							inout vec4 ambient,
							inout vec4 diffuse,
							inout vec4 specular)
{
	
	//add the lighting contributions
	ambient  += gl_LightSource[light].ambient * material[0];
	diffuse  += gl_LightSource[light].diffuse * material[1] *  NdotL;
	specular += gl_LightSource[light].specular * specularIn * iSpecular;
}