

void phong_pointLight(in int light, in float iSpecular, in float NdotL, in vec4 specularIn, inout MaterialProperties materialProp) {

    int lightIndex = dvd_lightIndex[light];
	//add the lighting contributions
    float att = _lightInfo[light]._attenuation;
	materialProp.ambient += dvd_LightSource[lightIndex]._diffuse.w * material[0] * att;
	if(NdotL > 0.0){
		materialProp.diffuse  += vec4(dvd_LightSource[lightIndex]._diffuse.rgb, 1.0)  * material[1] * NdotL * att;
		materialProp.specular += vec4(dvd_LightSource[lightIndex]._specular.rgb, 1.0) * specularIn  * iSpecular * att;
	}
}