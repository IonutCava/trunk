#ifndef _IMAGE_BASED_LIGHTING_FRAG_
#define _IMAGE_BASED_LIGHTING_FRAG_

#if defined(USE_PLANAR_REFLECTION)
layout(binding = TEXTURE_REFLECTION_PLANAR) uniform sampler2D texReflectPlanar;
#else //USE_PLANAR_REFLECTION
layout(binding = TEXTURE_REFLECTION_CUBE) uniform samplerCube texEnvironmentCube;
#endif //USE_PLANAR_REFLECTION

vec3 ImageBasedLighting(in vec3 colour, in vec3 normalWV, in float metallic, in float roughness, in uint textureSize, in uint probeIdx);

//ref: https://github.com/urho3d/Urho3D/blob/master/bin/CoreData/Shaders/GLSL/IBL.glsl
vec3 EnvBRDFApprox(in vec3 specularColor, in float roughness, in float NoV) {
    const vec4 c0 = vec4(-1.f, -0.0275f, -0.572f, 0.022f);
    const vec4 c1 = vec4(1.f, 0.0425f, 1.0f, -0.04f);
    const vec4 r = roughness * c0 + c1;
    const float a004 = min(r.x * r.x, exp2(-9.28f * NoV)) * r.x + r.y;
    const vec2 AB = vec2(-1.04f, 1.04f) * a004 + r.zw;

    return specularColor * AB.x + AB.y;
}

#if !defined(USE_PLANAR_REFLECTION)

vec3 GetSpecularDominantDir(in vec3 normal, in vec3 reflection, in float roughness) {
    const float smoothness = 1.0f - roughness;
    const float lerpFactor = smoothness * (sqrt(smoothness) + roughness);
    return mix(normal, reflection, lerpFactor);
}

vec3 FixCubeLookup(in vec3 v, in uint textureSize) {
    const float M = max(max(abs(v.x), abs(v.y)), abs(v.z));
    const float scale = (textureSize - 1) / textureSize;

    if (abs(v.x) != M) v.x += scale;
    if (abs(v.y) != M) v.y += scale;
    if (abs(v.z) != M) v.z += scale;

    return v;
}

//Box Projected Cube Environment Mapping by Bartosz Czuba
vec3 GetAdjustedReflectionWS(in vec3 posWS, in vec3 reflectionWS, in uint probeIdx) {
    if (probeIdx > 0) {
        const ProbeData probe = dvd_Probes[probeIdx - 1];
        const vec3 EnvBoxHalfSize = probe._halfExtents.xyz;
        const vec3 EnvBoxSize = EnvBoxHalfSize * 2;
        const vec3 EnvBoxStart = probe._positionW.xyz - EnvBoxHalfSize;
        
        const vec3 nrdir = normalize(reflectionWS);
        const vec3 rbmax = (EnvBoxStart + EnvBoxSize - posWS) / nrdir;
        const vec3 rbmin = (EnvBoxStart - posWS) / nrdir;

        const vec3 rbminmax = vec3(
            (nrdir.x > 0.f) ? rbmax.x : rbmin.x,
            (nrdir.y > 0.f) ? rbmax.y : rbmin.y,
            (nrdir.z > 0.f) ? rbmax.z : rbmin.z
        );
        const float fa = min(min(rbminmax.x, rbminmax.y), rbminmax.z);
        return (posWS + nrdir * fa) - (EnvBoxStart + EnvBoxHalfSize);
    }

    return normalize(reflectionWS);
}

#define GetMipFromRoughness(R) (R * 12.0f - pow(R, 6.0f) * 1.5f)

/// Calculate IBL contributation
///     normalW: surface normal in word space
///     roughness: surface roughness
///     textureSize: assuming square environment texture size, either widht or height
///     probeIdx: probe index associated with the current environment cube map (used for BPCEM)
vec3 IBL(in vec3 normalW, float roughness, in uint textureSize, in uint probeIdx) {
    roughness = max(roughness, 0.08f);
    const vec3 toCamera = (VAR._vertexW.xyz - dvd_cameraPosition.xyz);

#if 0
    const vec3 reflectVec = reflect(toCamera, normalW);
    const vec3 dominantDir = GetSpecularDominantDir(normalW, reflectVec, roughness);
    const vec3 fixedCubeDir = FixCubeLookup(dominantDir, textureSize);
    const vec3 bpcemDir = GetAdjustedReflectionWS(fixedCubeDir, probeIdx);

    return textureLod(texEnvironmentCube, bpcemDir, GetMipFromRoughness(roughness)).rgb;
#else
    const vec3 bpcemDir = GetAdjustedReflectionWS(VAR._vertexW.xyz, reflect(toCamera, normalW), probeIdx);
    return textureLod(texEnvironmentCube, bpcemDir, 0).rgb;
#endif
}
#else //!USE_PLANAR_REFLECTION
#define IBL(N, R, S, P) vec3(0.0f)
#endif //!USE_PLANAR_REFLECTION

#endif //_IMAGE_BASED_LIGHTING_FRAG_
