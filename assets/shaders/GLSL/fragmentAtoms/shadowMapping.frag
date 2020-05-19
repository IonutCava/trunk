#ifndef _SHADOW_MAPPING_FRAG_
#define _SHADOW_MAPPING_FRAG_

#if !defined(SHADOW_INTENSITY_FACTOR)
#define SHADOW_INTENSITY_FACTOR 1.0f
#endif

#if !defined(DISABLE_SHADOW_MAPPING)

layout(binding = SHADOW_SINGLE_MAP_ARRAY)  uniform sampler2DArray    singleDepthMaps;
layout(binding = SHADOW_LAYERED_MAP_ARRAY) uniform sampler2DArray    layeredDepthMaps;
layout(binding = SHADOW_CUBE_MAP_ARRAY)    uniform samplerCubeArray  cubeDepthMaps;

#include "shadowUtils.frag"

#define LT(n) n, n, n, n, n, n, n, n, n, n, n, n, n, n, n, n

bool detail_isInFrustum(in vec3 shadowCoord) {
    return shadowCoord.z <= 1.0f &&
        all(bvec4(shadowCoord.x >= 0.0f,
            shadowCoord.x <= 1.0f,
            shadowCoord.y >= 0.0f,
            shadowCoord.y <= 1.0f));
}

//Chebyshev Upper Bound
float detail_VSM(vec2 moments, float distance) {
    const float E_x2 = moments.y;
    const float Ex_2 = moments.x * moments.x;
    const float variance = max(E_x2 - Ex_2, dvd_MinVariance);
    const float mD = moments.x - distance;
    const float mD_2 = mD * mD;

    const float p = linstep(dvd_LightBleedBias, 1.0f, variance / (variance + mD_2));
    return saturate(max(p, distance <= moments.x ? 1.0f : 0.0f));
}

#endif

float detail_getShadowFactorDirectional(in uint shadowIndex, in float TanAcosNdotL) {
    const CSMShadowProperties properties = dvd_CSMShadowTransforms[shadowIndex];

    const vec4 crtDetails = properties.dvd_shadowLightDetails;
    const float bias = clamp(crtDetails.z * TanAcosNdotL, 0.f, 0.01f);

    const int Split = getCSMSlice(properties);
    const vec4 sc = properties.dvd_shadowLightVP[Split] * VAR._vertexW;
    const vec3 shadowCoord = sc.xyz / sc.w;
    if (detail_isInFrustum(shadowCoord)) {
        const vec2 moments = texture(layeredDepthMaps, vec3(shadowCoord.xy, float(Split + int(crtDetails.y)))).rg;
        const float ret = max(detail_VSM(moments, shadowCoord.z - bias), 0.2f);
        return 1.0f - (1.0f - saturate(ret / SHADOW_INTENSITY_FACTOR)) * crtDetails.w;
    }

    return 1.0f;
}

float detail_getShadowFactorSpot(in uint shadowIndex, in float TanAcosNdotL) {
    const SpotShadowProperties properties = dvd_SpotShadowTransforms[shadowIndex];

    const vec4 crtDetails = properties.dvd_shadowLightDetails;
    const float bias = clamp(crtDetails.z * TanAcosNdotL, 0.f, 0.01f);

    const vec4 sc = properties.dvd_shadowLightVP * VAR._vertexW;
    const vec3 shadowCoord = sc.xyz / sc.w;
    if (detail_isInFrustum(shadowCoord)) {
        const vec2 moments = texture(singleDepthMaps, vec3(shadowCoord.xy, crtDetails.y)).rg;
        const float ret = (moments.x > shadowCoord.z - bias) ? 1.0f : 0.0f;
        return 1.0f - (1.0f - saturate(ret / SHADOW_INTENSITY_FACTOR)) * crtDetails.w;
    }

    return 1.0f;
}

float detail_getShadowFactorPoint(in uint shadowIndex, in float TanAcosNdotL) {
    const PointShadowProperties properties = dvd_PointShadowTransforms[shadowIndex];

    const vec4 crtDetails = properties.dvd_shadowLightDetails;
    const float bias = clamp(crtDetails.z * TanAcosNdotL, 0.f, 0.01f);

    const float farPlane = properties.dvd_shadowLightPosition.w;
    // get vector between fragment position and light position
    const vec3 fragToLight = VAR._vertexW.xyz - properties.dvd_shadowLightPosition.xyz;
    // use the light to fragment vector to sample from the depth map 
    // it is currently in linear range between [0,1]. Re-transform back to original value
    const vec2 moments = texture(cubeDepthMaps, vec4(fragToLight, crtDetails.y)).rg * farPlane;
    // now get current linear depth as the length between the fragment and light position
    const float currentDepth = length(fragToLight) - bias;
    // now test for shadows
    return max(detail_VSM(moments, currentDepth), 0.2f);
}

float getShadowFactor(in ivec4 lightOptions, in float TanAcosNdotL) {
    float ret = 1.0f;
#if !defined(DISABLE_SHADOW_MAPPING)
    const int shadowIndex = lightOptions.y;
    if (dvd_receivesShadow && shadowIndex >= 0 && shadowIndex < MAX_SHADOW_CASTING_LIGHTS) {
        switch (lightOptions.x) {
            case 0:  {
                ret = detail_getShadowFactorDirectional(shadowIndex, TanAcosNdotL);
            } break;
            case 1: {
                ret = detail_getShadowFactorPoint(shadowIndex, TanAcosNdotL);
            } break;
            case 2 : {
                ret = detail_getShadowFactorSpot(shadowIndex, TanAcosNdotL);
            } break;
        }
    }
#endif
    return ret;

}
#endif //_SHADOW_MAPPING_FRAG_