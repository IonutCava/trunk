#ifndef _SHADOW_MAPPING_FRAG_
#define _SHADOW_MAPPING_FRAG_

#if !defined(SHADOW_INTENSITY_FACTOR)
#define SHADOW_INTENSITY_FACTOR 1.0f
#endif

#if !defined(DISABLE_SHADOW_MAPPING)

layout(binding = SHADOW_SINGLE_MAP_ARRAY)  uniform sampler2DArrayShadow    texDepthMapFromLight;
layout(binding = SHADOW_CUBE_MAP_ARRAY)    uniform samplerCubeArrayShadow  texDepthMapFromLightCube;
layout(binding = SHADOW_LAYERED_MAP_ARRAY) uniform sampler2DArray          texDepthMapFromLightArray;

#include "shadowUtils.frag"

#define LT(n) n, n, n, n, n, n, n, n, n, n, n, n, n, n, n, n

//Chebyshev Upper Bound
float VSM(vec2 moments, float distance) {
    const float E_x2 = moments.y;
    const float Ex_2 = moments.x * moments.x;
    const float variance = max(E_x2 - Ex_2, -(dvd_shadowingSettings.y));
    const float mD = (moments.x - distance);
    const float mD_2 = mD * mD;

    const float p = linstep(dvd_shadowingSettings.x, 1.0f, variance / (variance + mD_2));

    return saturate(max(p, distance <= moments.x ? 1.0f : 0.0f));
}

float getShadowFactorDirectional(in int idx, in int arrayOffset, in float bias, in float TanAcosNdotL) {
    const int Split = getCSMSlice(idx);

    const vec4 sc = dvd_shadowLightVP[Split + (idx * 6)] * VAR._vertexW;
    const vec3 shadowCoord = sc.xyz / sc.w;

    const bool inFrustum = shadowCoord.z <= 1.0f &&
        all(bvec4(shadowCoord.x >= 0.0f,
            shadowCoord.x <= 1.0f,
            shadowCoord.y >= 0.0f,
            shadowCoord.y <= 1.0f));

    float ret = 1.0f;
    if (inFrustum)
    {
        const float layer = float(Split + arrayOffset);

        vec2 moments = texture(texDepthMapFromLightArray, vec3(shadowCoord.xy, layer)).rg;

        //float shadowBias = DEPTH_EXP_WARP * exp(DEPTH_EXP_WARP * dvd_shadowingSettings.y);
        //float shadowWarpedz1 = exp(shadowCoord.z * DEPTH_EXP_WARP);
        //return mix(VSM(moments, shadowWarpedz1), 
        //             1.0, 
        //             clamp(((gl_FragCoord.z + dvd_shadowingSettings.z) - dvd_shadowingSettings.w) / dvd_shadowingSettings.z, 0.0, 1.0));

        //float bias = max(angleBias * (1.0 - dot(normal, lightDirection)), 0.0008);
        ret = max(VSM(moments, shadowCoord.z - clamp(bias * TanAcosNdotL, 0, 0.01f)), 0.2f);
    }

    return saturate(ret / SHADOW_INTENSITY_FACTOR);
}

float getShadowFactorDirectional(int idx, in float TanAcosNdotL) {
    if (dvd_receivesShadow && idx >= 0 && idx < MAX_SHADOW_CASTING_LIGHTS) {
        const vec4 crtDetails = dvd_shadowLightDetails[idx];
        return getShadowFactorDirectional(idx, int(crtDetails.y), crtDetails.z, TanAcosNdotL);
    }

    return 1.0f;
}


float getShadowFactorPoint(in int idx, in int arrayIndex, in float NdotL) {
    // SHADOW MAPS
    vec3 position_ls = dvd_shadowLightPosition[idx * 6].xyz;
    vec3 abs_position = abs(position_ls);
    float fs_z = -max(abs_position.x, max(abs_position.y, abs_position.z));
    vec4 clip = (dvd_shadowLightVP[idx * 6] * VAR._vertexW) * vec4(0.0, 0.0, fs_z, 1.0);
    float depth = (clip.z / clip.w) * 0.5 + 0.5;
    float ret = texture(texDepthMapFromLightCube, vec4(position_ls.xyz, arrayIndex), depth).r;
    //float bias = clamp(biasIn * TanAcosNdotL, 0, 0.01f);

    return saturate(ret / SHADOW_INTENSITY_FACTOR);
}

float getShadowFactorPoint(int idx, in float TanAcosNdotL) {
    if (dvd_receivesShadow && idx >= 0 && idx < MAX_SHADOW_CASTING_LIGHTS) {
        const vec4 crtDetails = dvd_shadowLightDetails[idx];
        return getShadowFactorPoint(idx, int(crtDetails.y), TanAcosNdotL);
    }
    return 1.0f;
}

float filterFinalShadow(in sampler2DArrayShadow shadowMap, in vec3 projCoords, in int shadowIndex, in float bias, float offset) {
    float visiblity = 1.0f;
    if (projCoords.x >= 1.0f || projCoords.x <= 0.0f ||
        projCoords.y >= 1.0f || projCoords.y <= 0.0f ||
        projCoords.z >= 1.0f || projCoords.z <= 0.0f) {
        visiblity = 1.0f;
    }
    else {
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
        }
        else {
            visiblity /= 5.0f;
        }
    }
    return visiblity;
}

float getShadowFactorSpot(in int idx, in int arrayOffset, in float bias, in float TanAcosNdotL) {
    const vec4 temp_coord = dvd_shadowLightVP[idx * 6] * VAR._vertexW;
    const vec3 shadow_coord = 0.5f + (temp_coord.xyz / temp_coord.w) * 0.5f;
    const float ret = filterFinalShadow(texDepthMapFromLight, shadow_coord, arrayOffset, clamp(bias * TanAcosNdotL, 0, 0.01f), 0.001f);
    return saturate(ret / SHADOW_INTENSITY_FACTOR);
}

float getShadowFactorSpot(int idx, in float TanAcosNdotL) {
    if (dvd_receivesShadow && idx >= 0 && idx < MAX_SHADOW_CASTING_LIGHTS) {
        const vec4 crtDetails = dvd_shadowLightDetails[idx];
        return getShadowFactorSpot(idx, int(crtDetails.y), crtDetails.z, TanAcosNdotL);
    }
    return 1.0f;
}
#else
float getShadowFactorDirectional(in uint idx, in vec4 details, in float TanAcosNdotL) {
    return 1.0f;
}
float getShadowFactorPoint(in uint idx, in int arrayOffset, in float TanAcosNdotL) {
    return 1.0f;
}
float getShadowFactorSpot(in uint idx, in int arrayOffset, in float bias, in float TanAcosNdotL) {
    return 1.0f;
}
#endif

#endif //_SHADOW_MAPPING_FRAG_