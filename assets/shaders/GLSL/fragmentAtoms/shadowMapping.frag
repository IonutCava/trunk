#ifndef _SHADOW_MAPPING_FRAG_
#define _SHADOW_MAPPING_FRAG_

#if !defined(SHADOW_INTENSITY_FACTOR)
#define SHADOW_INTENSITY_FACTOR 1.0f
#endif

#if !defined(DISABLE_SHADOW_MAPPING)

#if !defined(DISABLE_SHADOW_MAPPING_SPOT)
layout(binding = SHADOW_SINGLE_MAP_ARRAY)  uniform sampler2DArray    singleDepthMaps;
#endif //DISABLE_SHADOW_MAPPING_SPOT

#if !defined(DISABLE_SHADOW_MAPPING_CSM)
layout(binding = SHADOW_LAYERED_MAP_ARRAY) uniform sampler2DArray    layeredDepthMaps;
#endif //DISABLE_SHADOW_MAPPING_CSM

#if !defined(DISABLE_SHADOW_MAPPING_POINT)
layout(binding = SHADOW_CUBE_MAP_ARRAY)    uniform samplerCubeArray  cubeDepthMaps;
#endif //DISABLE_SHADOW_MAPPING_POINT

#include "shadowUtils.frag"

#define LT(n) n, n, n, n, n, n, n, n, n, n, n, n, n, n, n, n

bool detail_isInFrustum(in vec3 shadowCoord) {
    return shadowCoord.z <= 1.0f &&
           all(bvec4(shadowCoord.x >= 0.0f,
                     shadowCoord.x <= 1.0f,
                     shadowCoord.y >= 0.0f,
                     shadowCoord.y <= 1.0f));
}

float detail_ChebyshevUpperBound(vec2 moments, float distance) {
    // Compute and clamp minimum variance to avoid numeric issues that may occur during filtering
    const float variance = max(moments.y - moments.x * moments.x, dvd_MinVariance);
    // Compute probabilistic upper bound.
    const float mD = moments.x - distance;
    const float p_max = variance / (variance + mD * mD);

    // Reduce light bleed
    const float p = linstep(dvd_LightBleedBias, 1.0f, p_max);

    // No saturate() yet. We need to adjust using SHADOW_INTENSITY_FACTOR later and then clamp to 01
    return max(p, distance <= moments.x ? 1.0f : 0.0f);
}

float detail_getShadowFactorDirectional(in uint shadowIndex, in float TanAcosNdotL) {
#if !defined(DISABLE_SHADOW_MAPPING_CSM)
    const CSMShadowProperties properties = dvd_CSMShadowTransforms[shadowIndex];

    const int Split = getCSMSlice(properties.dvd_shadowLightPosition);
    const vec4 sc = properties.dvd_shadowLightVP[Split] * VAR._vertexW;
    const vec3 shadowCoord = sc.xyz / sc.w;
    if (detail_isInFrustum(shadowCoord)) 
    {
        const vec4 crtDetails = properties.dvd_shadowLightDetails;
        const float bias = clamp(crtDetails.z * TanAcosNdotL, 0.f, 0.01f);
        // now get current linear depth as the length between the fragment and light position
        const float currentDepth = shadowCoord.z - bias;
        const vec2 moments = texture(layeredDepthMaps, vec3(shadowCoord.xy, crtDetails.y + Split)).rg;
        const float ret = detail_ChebyshevUpperBound(moments, currentDepth) * SHADOW_INTENSITY_FACTOR;
        return saturate(ret * crtDetails.w);
    }

    return 1.0f;
#else
    return 1.0f;
#endif
}

float detail_getShadowFactorSpot(in uint shadowIndex, in float TanAcosNdotL) {
#if !defined(DISABLE_SHADOW_MAPPING_SPOT)
    const SpotShadowProperties properties = dvd_SpotShadowTransforms[shadowIndex];

    const vec4 shadowCoords = properties.dvd_shadowLightVP * VAR._vertexW;

    const vec4 crtDetails = properties.dvd_shadowLightDetails;

    const float fragToLight = length(VAR._vertexW.xyz - properties.dvd_shadowLightPosition.xyz);
    const vec2 moments = texture(singleDepthMaps, vec3(shadowCoords.xy / shadowCoords.w, crtDetails.y)).rg;

    const float bias = clamp(crtDetails.z * TanAcosNdotL, 0.f, 0.01f);
    const float farPlane = properties.dvd_shadowLightPosition.w;
    const float ret = detail_ChebyshevUpperBound(moments, (fragToLight / farPlane) - bias) * SHADOW_INTENSITY_FACTOR;
    return saturate(ret * crtDetails.w);
#else
    return 1.0f;
#endif
}

float detail_getShadowFactorPoint(in uint shadowIndex, in float TanAcosNdotL) {
#if !defined(DISABLE_SHADOW_MAPPING_POINT)
    const PointShadowProperties properties = dvd_PointShadowTransforms[shadowIndex];

    const vec4 crtDetails = properties.dvd_shadowLightDetails;
    // get vector between fragment position and light position
    const vec3 fragToLight = VAR._vertexW.xyz - properties.dvd_shadowLightPosition.xyz;
    // use the light to fragment vector to sample from the depth map 
    const vec2 moments = texture(cubeDepthMaps, vec4(fragToLight, crtDetails.y)).rg;

    const float bias = clamp(crtDetails.z * TanAcosNdotL, 0.f, 0.01f);
    const float farPlane = properties.dvd_shadowLightPosition.w;
    const float ret = detail_ChebyshevUpperBound(moments, (length(fragToLight) / farPlane) - bias) * SHADOW_INTENSITY_FACTOR;
    return saturate(ret * crtDetails.w);
#else
    return 1.0f;
#endif
}

float getShadowFactorDirectional(in int shadowIndex, in float TanAcosNdotL) {
    return shadowIndex >= 0 ? detail_getShadowFactorDirectional(shadowIndex, TanAcosNdotL) : 1.0f;
}

float getShadowFactorPoint(in int shadowIndex, in float TanAcosNdotL) {
    return shadowIndex >= 0 ? detail_getShadowFactorPoint(shadowIndex, TanAcosNdotL) : 1.0f;
}

float getShadowFactorSpot(in int shadowIndex, in float TanAcosNdotL) {
    return shadowIndex >= 0 ? detail_getShadowFactorSpot(shadowIndex, TanAcosNdotL) : 1.0f;
}

#else //DISABLE_SHADOW_MAPPING

int getCSMSlice(in vec4 props[MAX_CSM_SPLITS_PER_LIGHT]) {
    return 0;
}

float getShadowFactorDirectional(in int shadowIndex, in float TanAcosNdotL) {
    return 1.0f;
}
float getShadowFactorPoint(in int shadowIndex, in float TanAcosNdotL) {
    return 1.0f;
}
float getShadowFactorSpot(in int shadowIndex, in float TanAcosNdotL) {
    return 1.0f;
}
#endif //DISABLE_SHADOW_MAPPING

#endif //_SHADOW_MAPPING_FRAG_