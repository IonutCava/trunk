#ifndef _PHONG_LIGHTING_FRAG_
#define _PHONG_LIGHTING_FRAG_

vec4 Phong(in vec4 lightColourAndAtt,
           in vec3 lightDirection,// direction is NOT normalized
           in vec3 albedo,
           in vec4 specular)
{
    float NDotL = max(dot(normalize(lightDirection), private_normalWV), 0.0);
    vec3 colourOut = clamp(lightColourAndAtt.rgb * albedo * NDotL * lightColourAndAtt.a, 0.0, 1.0);

    if (NDotL > 0.0f) {
        vec3 dvd_ViewDirNorm = normalize(-VAR._vertexWV.xyz);
#if defined(USE_SHADING_BLINN_PHONG)
        vec3 halfDir = normalize(normalize(lightDirection) + dvd_ViewDirNorm);
        float specAngle = max(dot(halfDir, private_normalWV), 0.0);
#else
        vec3 reflectDir = reflect(normalize(-lightDirection), private_normalWV);
        float specAngle = max(dot(reflectDir, dvd_ViewDirNorm), 0.0);
#endif
        float shininess = pow(specAngle, specular.a) * lightColourAndAtt.a;
        colourOut += clamp(lightColourAndAtt.rgb * specular.rgb * shininess, 0.0, 1.0);

        return vec4(colourOut, shininess);
    }

    return vec4(colourOut, 0.0f);
}


#endif //_PHONG_LIGHTING_FRAG_