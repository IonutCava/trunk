#ifndef _IMAGE_BASED_LIGHTING_FRAG_
#define _IMAGE_BASED_LIGHTING_FRAG_

#if defined(USE_PLANAR_REFLECTION)
layout(binding = TEXTURE_REFLECTION_PLANAR) uniform sampler2D texReflectPlanar;
#else //USE_PLANAR_REFLECTION
layout(binding = TEXTURE_REFLECTION_CUBE) uniform samplerCube texEnvironmentCube;
#endif //USE_PLANAR_REFLECTION

//ref: https://github.com/urho3d/Urho3D/blob/master/bin/CoreData/Shaders/GLSL/IBL.glsl
float GetMipFromRoughness(float roughness) {
    return (roughness * 12.0f - pow(roughness, 6.0f) * 1.5f);
}

vec3 GetSpecularDominantDir(vec3 normal, vec3 reflection, float roughness) {
    float smoothness = 1.0 - roughness;
    float lerpFactor = smoothness * (sqrt(smoothness) + roughness);
    return mix(normal, reflection, lerpFactor);
}

vec3 EnvBRDFApprox(vec3 SpecularColor, float Roughness, float NoV) {
    vec4 c0 = vec4(-1.f, -0.0275f, -0.572f, 0.022f);
    vec4 c1 = vec4(1.f, 0.0425f, 1.0f, -0.04f);
    vec4 r = Roughness * c0 + c1;
    float a004 = min(r.x * r.x, exp2(-9.28f * NoV)) * r.x + r.y;
    vec2 AB = vec2(-1.04f, 1.04f) * a004 + r.zw;
    return SpecularColor * AB.x + AB.y;
}

vec3 FixCubeLookup(in vec3 v, in uint textureSize) {
    float M = max(max(abs(v.x), abs(v.y)), abs(v.z));
    float scale = (textureSize - 1) / textureSize;

    if (abs(v.x) != M) v.x += scale;
    if (abs(v.y) != M) v.y += scale;
    if (abs(v.z) != M) v.z += scale;

    return v;
}

/// Calculate IBL contributation
///     reflectVec: reflection vector for cube sampling
///     wsNormal: surface normal in word space
///     toCamera: normalized direction from surface point to camera
///     roughness: surface roughness
///     ambientOcclusion: ambient occlusion
vec3 ImageBasedLighting(vec3 reflectVec, vec3 wsNormal, vec3 toCamera, float roughness, in uint textureSize) {
#if defined(USE_PLANAR_REFLECTION)
    return vec3(0.0f);
#else
    roughness = max(roughness, 0.08);
    reflectVec = GetSpecularDominantDir(wsNormal, reflectVec, roughness);

    const float ndv = clamp(dot(-toCamera, wsNormal), 0.0, 1.0);
    const float mipSelect = GetMipFromRoughness(roughness);

    return textureLod(texEnvironmentCube, FixCubeLookup(reflectVec, textureSize), mipSelect).rgb;
#endif
}
#endif //_IMAGE_BASED_LIGHTING_FRAG_