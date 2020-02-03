#ifndef _SHADOW_SPOT_FRAG_
#define _SHADOW_SPOT_FRAG_

float filterFinalShadow(in sampler2DArrayShadow shadowMap, in vec3 projCoords, in int shadowIndex, in float bias, float offset){
    float visiblity = 1.0f;
    if (projCoords.x >= 1.0f || projCoords.x <= 0.0f ||
        projCoords.y >= 1.0f || projCoords.y <= 0.0f ||
        projCoords.z >= 1.0f || projCoords.z <= 0.0f) {
        visiblity = 1.0f;
    } else {
        vec2 offsetArray1[4] = vec2[](vec2(-offset, -offset), vec2(-offset, offset), vec2(offset, offset), vec2(offset, -offset));
        visiblity = texture(shadowMap, vec4(projCoords.xy, shadowIndex, projCoords.z - bias));
        for (int i = 0; i < 4; i++) {
            visiblity += texture(shadowMap, vec4(projCoords.xy + offsetArray1[i], shadowIndex, projCoords.z - bias));
        }
        if (visiblity < 5.0f) {
            vec2 offsetArray2[4] = vec2[](vec2(0, offset), vec2(offset, 0), vec2(0, -offset), vec2(-offset, 0));
            for (int i = 0; i < 4; i++) {
                visiblity += texture(shadowMap, vec4(projCoords.xy + offsetArray2[i], shadowIndex, projCoords.z - bias));
            }
            visiblity /= 9.0f;
        } else {
            visiblity /= 5.0f;
        }
    }
    return visiblity;
}

float getShadowFactorSpot(in uint idx, in vec4 details) {

    vec4 temp_coord = dvd_shadowLightVP[idx * 6] * VAR._vertexW;
    vec3 shadow_coord = temp_coord.xyz / temp_coord.w;
    shadow_coord = 0.5f + shadow_coord * 0.5f;

    return filterFinalShadow(texDepthMapFromLight, shadow_coord, int(details.y), details.z, 0.001f);
}

#endif //_SHADOW_SPOT_FRAG_