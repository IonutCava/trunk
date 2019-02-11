#ifndef _PHONG_LIGHTING_FRAG_
#define _PHONG_LIGHTING_FRAG_

void Phong(in vec3 lightColour,
           in vec3 lightDirection,// direction is NOT normalized
           in float att,
           in vec3 normalWV,
           in vec3 albedo,
           in vec3 specular,
           in float reflectivity,
           inout vec3 colourInOut,
           inout float reflectionCoeff)
{
    float NDotL = max(dot(normalize(lightDirection), normalWV), 0.0);
    colourInOut += clamp(lightColour * albedo * NDotL * att, 0.0, 1.0);

    if (NDotL > 0.0f) {
        vec3 dvd_ViewDirNorm = normalize(-VAR._vertexWV.xyz);
#if defined(USE_SHADING_BLINN_PHONG)
        vec3 halfDir = normalize(normalize(lightDirection) + dvd_ViewDirNorm);
        float specAngle = max(dot(halfDir, normalWV), 0.0);
#else
        vec3 reflectDir = reflect(normalize(-lightDirection), normalWV);
        float specAngle = max(dot(reflectDir, dvd_ViewDirNorm), 0.0);
#endif
        float shininess = pow(specAngle, reflectivity) * att;
        colourInOut += clamp(lightColour * specular * shininess, 0.0, 1.0);

        reflectionCoeff = saturate(reflectionCoeff + shininess);
    }
}


#endif //_PHONG_LIGHTING_FRAG_