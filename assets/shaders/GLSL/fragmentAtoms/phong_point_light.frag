

void phong_pointLight(in int light, 
					  in vec4 specularIn, 
					  in float iSpecular,
					  in float NdotL,
					  inout vec4 ambient,
					  inout vec4 diffuse,
					  inout vec4 specular)
{

	//add the lighting contributions
	ambient += material[0] * gl_LightSource[light].ambient * _attenuation[light];
	diffuse += material[1] * gl_LightSource[light].diffuse * NdotL * _attenuation[light];
	specular += specularIn * gl_LightSource[light].specular * iSpecular * _attenuation[light];
}