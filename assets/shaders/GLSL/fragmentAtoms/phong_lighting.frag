#ifndef _PHONG_LIGHTING_FRAG_
#define _PHONG_LIGHTING_FRAG_

vec4 Phong(in vec4 lightColourAndAtt,
           in vec4 specular,
           in vec4 albedoAndShadow,
           in vec3 lightDirection) // direction is NOT normalized
{
    vec3 albedo = albedoAndShadow.rgb;

    const float NDotL = max(dot(normalize(lightDirection), private_normalWV), 0.0);
    vec3 colourOut = clamp(lightColourAndAtt.rgb * albedo * NDotL * lightColourAndAtt.a, 0.0, 1.0);

    if (NDotL > 0.0f && albedoAndShadow.a > 0.2) {
        const vec3 dvd_ViewDirNorm = normalize(-VAR._vertexWV.xyz);
#if defined(USE_SHADING_BLINN_PHONG)
        const vec3 halfDir = normalize(normalize(lightDirection) + dvd_ViewDirNorm);
        const float specAngle = max(dot(halfDir, private_normalWV), 0.0);
#else
        const vec3 reflectDir = reflect(normalize(-lightDirection), private_normalWV);
        const float specAngle = max(dot(reflectDir, dvd_ViewDirNorm), 0.0);
#endif
        const float shininess = pow(specAngle, specular.a) * lightColourAndAtt.a;
        colourOut += clamp(lightColourAndAtt.rgb * specular.rgb * shininess, 0.0, 1.0);

        return vec4(colourOut, shininess);
    }

    return vec4(colourOut, 0.0f);
}


#endif //_PHONG_LIGHTING_FRAG_