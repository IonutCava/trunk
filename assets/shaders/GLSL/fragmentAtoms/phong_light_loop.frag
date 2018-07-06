#include "shadowMapping.frag"

void phong_omni(in uint lightIndex, in float NdotL, in float iSpecular, float att, inout MaterialProperties materialProp) {
    //add the lighting contributions
    materialProp.ambient += dvd_MatAmbient * dvd_LightSource[lightIndex]._diffuse.a * att;
    if (NdotL > 0.0){
        materialProp.diffuse  += dvd_LightSource[lightIndex]._diffuse.rgb * dvd_MatDiffuse.rgb * NdotL * att;
        materialProp.specular += dvd_LightSource[lightIndex]._specular.rgb * materialProp.specularValue * iSpecular * att;
    }
}

void phong_spot(in uint lightIndex, in float NdotL, in float iSpecular, inout MaterialProperties materialProp) {
    //Diffuse intensity
    if (NdotL > 0.0){
        float att = _lightInfo[lightIndex]._attenuation;
        //add the lighting contributions
        materialProp.ambient  += dvd_MatAmbient * dvd_LightSource[lightIndex]._diffuse.w  * att;
        materialProp.diffuse  += dvd_LightSource[lightIndex]._diffuse.rgb  * dvd_MatDiffuse.rgb * NdotL * att;
        materialProp.specular += dvd_LightSource[lightIndex]._specular.rgb * materialProp.specularValue  * iSpecular * att;
    }
}

void phong_loop(in vec3 normal, inout MaterialProperties materialProp){
    vec3 L, R; 
    vec3 viewDirection = normalize(_viewDirection);
    float iSpecular = 0.0;
    // Apply all lighting contributions
    for (uint i = 0; i < MAX_LIGHTS_PER_SCENE; i++){
        if (_lightCount == i) break;

        L = normalize(_lightInfo[i]._lightDirection);
        R = normalize(-reflect(L, normal));
        iSpecular = clamp(pow(max(dot(R, viewDirection), 0.0), dvd_MatShininess), 0.0, 1.0);
        switch (uint(dvd_LightSource[i]._position.w)){
            case LIGHT_DIRECTIONAL      : 
            case LIGHT_OMNIDIRECTIONAL  : phong_omni(i, max(dot(normal, L), 0.0), iSpecular, _lightInfo[i]._attenuation, materialProp); break;
            case LIGHT_SPOT             : phong_spot(i, max(dot(normal, L), 0.0), iSpecular, materialProp); break;
        }
    }

    materialProp.diffuse  = clamp(materialProp.diffuse,  vec3(0.0), vec3(1.0));
    materialProp.ambient  = clamp(materialProp.ambient,  vec3(0.0), vec3(1.0));
    materialProp.specular = clamp(materialProp.specular, vec3(0.0), vec3(1.0));
 }

float shadow_loop(){
    // Early-out if shadows are disabled
    if (!dvd_shadowMapping) return 1.0;
       
    float shadow = 1.0;
    for (uint i = 0; i < MAX_SHADOW_CASTING_LIGHTS; i++){
        if (_lightCount == i) break;

        if (dvd_LightSource[i]._options.x == 1)
            switch (uint(dvd_LightSource[i]._position.w)){
                case LIGHT_DIRECTIONAL     : shadow *= applyShadowDirectional(i, dvd_ShadowSource[i]); break;
                case LIGHT_OMNIDIRECTIONAL : shadow *= applyShadowPoint(i, dvd_ShadowSource[i]);       break;
                case LIGHT_SPOT            : shadow *= applyShadowSpot(i, dvd_ShadowSource[i]);        break;
            }
    }

    return clamp(shadow, 0.2, 1.0);
}