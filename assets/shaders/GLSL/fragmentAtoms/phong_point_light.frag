

void phong_pointLight(in int light, 
					  in float iSpecular,
					  in float NdotL,
					  in vec4 specularIn,
					  inout MaterialProperties materialProp)
{
    int lightIndex = dvd_lightIndex[light];
	//add the lighting contributions
	materialProp.ambient += dvd_LightSource[lightIndex]._diffuse.w * material[0] * _attenuation[light];
	if(NdotL > 0.0){
		materialProp.diffuse  += vec4(dvd_LightSource[lightIndex]._diffuse.rgb, 1.0)  * material[1] * NdotL * _attenuation[light];
		materialProp.specular += vec4(dvd_LightSource[lightIndex]._specular.rgb, 1.0) * specularIn  * iSpecular * _attenuation[light];
	}
}