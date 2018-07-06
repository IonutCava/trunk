
void phong_directionalLight(in int light, in float iSpecular, in float NdotL, in vec4 specularIn, inout MaterialProperties materialProp) { 

    int lightIndex = dvd_lightIndex[light];
	//add the lighting contributions
	materialProp.ambient  += material[0] * dvd_LightSource[lightIndex]._diffuse.w;
	if(NdotL > 0.0){
		materialProp.diffuse  += vec4(dvd_LightSource[lightIndex]._diffuse.rgb, 1.0) * material[1] *  NdotL;
		materialProp.specular += vec4(dvd_LightSource[lightIndex]._specular.rgb, 1.0) * specularIn * iSpecular;
	}
}