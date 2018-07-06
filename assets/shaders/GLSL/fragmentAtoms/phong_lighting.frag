#ifndef _PHONG_LIGHTING_FRAG_
#define _PHONG_LIGHTING_FRAG_

void phong_omni(const in vec4 lightColor, const in LightPropertiesFrag lightProp, inout MaterialProperties materialProp) {
    //add the lighting contributions
	float att = lightProp._att;
    materialProp.ambient += dvd_MatAmbient * lightColor.a * att;
    if (lightProp._NDotL > 0.0) {
#if defined(USE_SHADING_BLINN_PHONG)
		vec3 halfDir = normalize(lightProp._lightDir + lightProp._viewDirNorm);
		float specAngle = max(dot(lightProp._normal, halfDir), 0.0);
#else
		vec3 reflectDir = reflect(-lightProp._lightDir, lightProp._normal);
		float specAngle = max(dot(reflectDir, lightProp._viewDirNorm), 0.0);
#endif
        materialProp.diffuse  += lightColor.rgb * dvd_MatDiffuse.rgb * lightProp._NDotL * att;
        materialProp.specular += lightColor.rgb * materialProp.specularColor * pow(specAngle, dvd_MatShininess) * att;
    }
}

void phong_spot(const in vec4 lightColor, const in LightPropertiesFrag lightProp, inout MaterialProperties materialProp) {
    //Diffuse intensity
    if (lightProp._NDotL > 0.0){
	    //add the lighting contributions
		float att = lightProp._att;
#if defined(USE_SHADING_BLINN_PHONG)
		vec3 halfDir = normalize(lightProp._lightDir + lightProp._viewDirNorm);
		float specAngle = max(dot(lightProp._normal, halfDir), 0.0);
#else
		vec3 reflectDir = reflect(-lightProp._lightDir, lightProp._normal);
		float specAngle = max(dot(reflectDir, lightProp._viewDirNorm), 0.0);
#endif
        materialProp.ambient  += dvd_MatAmbient * lightColor.a * att;
        materialProp.diffuse  += lightColor.rgb * dvd_MatDiffuse.rgb * lightProp._NDotL * att;
        materialProp.specular += lightColor.rgb * materialProp.specularColor * pow(specAngle, dvd_MatShininess) * att;
    }
}

void Phong(const in LightPropertiesFrag lightProp, inout MaterialProperties materialProp) {
    switch (dvd_LightSource[lightProp._lightIndex]._options.x) {
        case LIGHT_DIRECTIONAL :
		case LIGHT_OMNIDIRECTIONAL : {
            phong_omni(dvd_LightSource[lightProp._lightIndex]._color, 
					   lightProp,
					   materialProp);
        } break;
        case LIGHT_SPOT : {
            phong_spot(dvd_LightSource[lightProp._lightIndex]._color, 
					   lightProp,
					   materialProp);
        } break;
    }
}


#endif //_PHONG_LIGHTING_FRAG_