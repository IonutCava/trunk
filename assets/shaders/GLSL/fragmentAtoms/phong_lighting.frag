#ifndef _PHONG_LIGHTING_FRAG_
#define _PHONG_LIGHTING_FRAG_

void Phong(const in LightPropertiesFrag lightProp, inout MaterialProperties materialProp, in vec3 normal) {
    if (lightProp._NDotL > 0.0) {
        vec4 lightColor = dvd_LightSource[lightProp._lightIndex]._color;
        //add the lighting contributions
        float att = lightProp._att;
#if defined(USE_SHADING_BLINN_PHONG)
        vec3 halfDir = normalize(lightProp._lightDir + lightProp._viewDirNorm);
        float specAngle = max(dot(normal, halfDir), 0.0);
#else
        vec3 reflectDir = reflect(-lightProp._lightDir, normal);
        float specAngle = max(dot(reflectDir, lightProp._viewDirNorm), 0.0);
#endif
        materialProp.diffuse += lightColor.rgb * dvd_MatDiffuse.rgb * lightProp._NDotL * att;
        materialProp.specular += lightColor.rgb * materialProp.specularColor * pow(specAngle, dvd_MatShininess) * att;
    }
}


#endif //_PHONG_LIGHTING_FRAG_