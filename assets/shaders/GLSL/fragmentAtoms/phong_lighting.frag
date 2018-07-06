#ifndef _PHONG_LIGHTING_FRAG_
#define _PHONG_LIGHTING_FRAG_

void Phong(in int lightIndex, in vec3 normalWV, inout vec3 colourInOut) {
    // direction is NOT normalized
    vec3 lightDirection = getLightDirection(lightIndex);
    float att = getLightAttenuation(lightIndex, lightDirection);

    vec3 lightColour = dvd_LightSource[lightIndex]._colour.rgb;

    float NDotL = max(dot(normalize(lightDirection), normalWV), 0.0);
    colourInOut += clamp(lightColour * dvd_MatDiffuse.rgb * NDotL * att, 0.0, 1.0);

    if (NDotL > 0.0) {
        vec3 dvd_ViewDirNorm = normalize(-VAR._vertexWV.xyz);
#if defined(USE_SHADING_BLINN_PHONG)
        vec3 halfDir = normalize(normalize(lightDirection) + dvd_ViewDirNorm);
        float specAngle = max(dot(halfDir, normalWV), 0.0);
#else
        vec3 reflectDir = reflect(normalize(-lightDirection), normalWV);
        float specAngle = max(dot(reflectDir, dvd_ViewDirNorm), 0.0);
#endif
        float shininess = pow(specAngle, dvd_MatShininess) * att;
        colourInOut += clamp(lightColour * dvd_MatSpecular.rgb * shininess, 0.0, 1.0);
    }
}


#endif //_PHONG_LIGHTING_FRAG_