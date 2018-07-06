void phong_spotLight(in int light, 
					 in float iSpecular,
					 in float NdotL,
					 in vec4 specularIn,
					 inout MaterialProperties materialProp)
{

	//Diffuse intensity
	if(NdotL > 0.0){
		//add the lighting contributions
		materialProp.ambient  += gl_LightSource[light].ambient  * material[0] *  _attenuation[light];
		materialProp.diffuse  += gl_LightSource[light].diffuse  * material[1] * NdotL * _attenuation[light];;
		materialProp.specular += gl_LightSource[light].specular * specularIn  * iSpecular * _attenuation[light];
		
	}
}