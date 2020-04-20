#ifndef _PHONG_LIGHTING_FRAG_
#define _PHONG_LIGHTING_FRAG_

vec4 Phong(in vec3 lightDirectionWV, 
           in vec4 lightColourAndAtt,
           in vec4 specularShininess,
           in vec4 albedoAndShadow,
           in vec3 normalWV)
{
    const float kPi = 3.14159265;
    const float kShininess = specularShininess.a;

    const vec3 lightColour = lightColourAndAtt.rgb;
    const float attenuation = lightColourAndAtt.a;
    const float shadowFactor = albedoAndShadow.a;
    const float lambertian = saturate(dot(lightDirectionWV, normalWV));

    float specularCoefficient = 0.0;
    if (lambertian > 0.0f) {
        const vec3 viewDir = normalize(VAR._viewDirectionWV);

#if defined(USE_SHADING_BLINN_PHONG)
        const float kEnergyConservation = (8.0 + kShininess) / (8.0 * kPi);

        const vec3 halfDir = normalize(lightDirectionWV + viewDir);
        specularCoefficient = kEnergyConservation * pow(max(dot(halfDir, normalWV), 0.0), kShininess);
#else
        const float kEnergyConservation = (2.0 + kShininess) / (2.0 * kPi);

        const vec3 reflectDir = reflect(-lightDirectionWV, normalWV);
        specularCoefficient = kEnergyConservation * pow(max(dot(reflectDir, viewDir), 0.0), kShininess);
#endif
    }
    const vec3 diffuse = lambertian * albedoAndShadow.rgb * lightColour;
    const vec3 specular = specularCoefficient * specularShininess.rgb * lightColour;
    const vec3 colourOut = attenuation * (diffuse + specular);

    return vec4(colourOut * shadowFactor, specularCoefficient);
}


#endif //_PHONG_LIGHTING_FRAG_