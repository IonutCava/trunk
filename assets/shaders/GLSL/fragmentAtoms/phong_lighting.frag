#ifndef _PHONG_LIGHTING_FRAG_
#define _PHONG_LIGHTING_FRAG_

void Phong(in uint lightIndex, in vec3 normal, inout vec3 colorInOut, inout vec3 specularInOut) {
    vec4 dirAndDot = getLightDirectionAndDot(lightIndex, normal);

    if (dirAndDot.w > 0.0) {
        vec4 lightColor = dvd_LightSource[lightIndex]._color;
        //add the lighting contributions
        float att = getLightAttenuation(lightIndex, dirAndDot.xyz);
#if defined(USE_SHADING_BLINN_PHONG)
        vec3 halfDir = normalize(dirAndDot.xyz + dvd_ViewDirNormAndDist.xyz);
        float specAngle = max(dot(normal, halfDir), 0.0);
#else
        vec3 reflectDir = reflect(-dirAndDot.xyz, normal);
        float specAngle = max(dot(reflectDir, dvd_ViewDirNormAndDist.xyz), 0.0);
#endif
        colorInOut += lightColor.rgb * dvd_MatDiffuse.rgb * dirAndDot.w * att;
        specularInOut += lightColor.rgb * dvd_MatSpecular.rgb * pow(specAngle, dvd_MatShininess) * att;
    }
}


#endif //_PHONG_LIGHTING_FRAG_