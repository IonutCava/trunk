

void phong_pointLight(in int light, 
					  in float iSpecular,
					  in float NdotL,
					  in vec4 specularIn,
					  inout MaterialProperties materialProp)
{

	//add the lighting contributions
	materialProp.ambient += gl_LightSource[light].ambient * material[0] * _attenuation[light];
	if(NdotL > 0.0){
		materialProp.diffuse  += gl_LightSource[light].diffuse  * material[1] * NdotL * _attenuation[light];
		materialProp.specular += gl_LightSource[light].specular * specularIn  * iSpecular * _attenuation[light];
	}
}