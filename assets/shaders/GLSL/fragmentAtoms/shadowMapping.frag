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
}

float detail_getShadowFactorSpot(in uint shadowIndex, in float TanAcosNdotL) {
    const SpotShadowProperties properties = dvd_SpotShadowTransforms[shadowIndex];

    const vec4 shadowCoords = properties.dvd_shadowLightVP * VAR._vertexW;

    const vec4 crtDetails = properties.dvd_shadowLightDetails;

    const float fragToLight = length(VAR._vertexW.xyz - properties.dvd_shadowLightPosition.xyz);
    const vec2 moments = texture(singleDepthMaps, vec3(shadowCoords.xy / shadowCoords.w, crtDetails.y)).rg;

    const float bias = clamp(crtDetails.z * TanAcosNdotL, 0.f, 0.01f);
    const float farPlane = properties.dvd_shadowLightPosition.w;
    const float ret = detail_ChebyshevUpperBound(moments, (fragToLight / farPlane) - bias) * SHADOW_INTENSITY_FACTOR;
    return saturate(ret * crtDetails.w);
}

float detail_getShadowFactorPoint(in uint shadowIndex, in float TanAcosNdotL) {
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
}

bool CanReceiveShadows(in int shadowIndex, in bool receivesShadows, in int lodLevel) {
    return receivesShadows &&
           shadowIndex >= 0 &&
           shadowIndex < MAX_SHADOW_CASTING_LIGHTS &&
           lodLevel <= 2;
}
#else //DISABLE_SHADOW_MAPPING
int getCSMSlice(in vec4 props[MAX_CSM_SPLITS_PER_LIGHT]) {
    return 0;
}
#endif //DISABLE_SHADOW_MAPPING

float getShadowFactor(in ivec4 lightOptions, in float TanAcosNdotL, in bool receivesShadows, in int lodLevel) {
    float ret = 1.0f;

#if !defined(DISABLE_SHADOW_MAPPING)
    const int shadowIndex = lightOptions.y;
    if (CanReceiveShadows(shadowIndex, receivesShadows, lodLevel)) {
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