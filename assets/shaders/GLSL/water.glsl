-- Vertex

#include "vbInputData.vert"
#include "lightingDefaults.vert"

layout(location = 0) out flat int _underwater;
layout(location = 1) out      vec3 _incident;

void main(void)
{
    computeData();
    
    computeLightVectors(dvd_NormalMatrixWV(DATA_IDX));

    _underwater = dvd_cameraPosition.y < VAR._vertexW.y ? 1 : 0;

    _incident = (dvd_InverseViewMatrix * VAR._vertexWV).xyz;

    gl_Position = VAR._vertexWVP;
}

-- Fragment

#define SHADOW_INTENSITY_FACTOR 0.5f

layout(location = 0) in flat int _underwater;
layout(location = 1) in      vec3 _incident;
uniform vec2 _noiseTile;
uniform vec2 _noiseFactor;

#define CUSTOM_IBL
#define USE_SHADING_BLINN_PHONG
#define USE_DEFERRED_NORMALS
#define USE_PLANAR_REFLECTION
#define USE_PLANAR_REFRACTION

#if defined(PRE_PASS)
#include "prePass.frag"
#else // PRE_PASS
#include "BRDF.frag"
#include "output.frag"

const float Eta = 0.15f; //water

// Use metalness as a bias towards extra reflectivity (for artistic purposes)
float Fresnel(in vec3 viewDir, in vec3 normal, in float metalness) {
    const float fresnel = Eta + (1.0f - Eta) * pow(max(0.0f, 1.0f - dot(-viewDir, normal)), 5.0f);
    return saturate(fresnel + metalness);
}

vec3 _private_reflect = vec3(0.f);

vec3 ImageBasedLighting(in vec3 colour, in vec3 normalWV, in float metallic, in float roughness, in int textureSize) {
    // This will actually return the fresnel'ed mixed between reflection and refraction as that's more useful for debugging
    return _private_reflect;
}
#endif // PRE_PASS

void main()
{
#if defined(PRE_PASS)
    const float kDistortion = 0.015f;
    const float kRefraction = 0.09f;

    float time2 = float(dvd_time) * 0.00001f;
    vec2 uvNormal0 = VAR._texCoord * _noiseTile;
    uvNormal0.s += time2;
    uvNormal0.t += time2;
    vec2 uvNormal1 = VAR._texCoord * _noiseTile;
    uvNormal1.s -= time2;
    uvNormal1.t += time2;

    vec3 normal0 = getBump(uvNormal0);
    vec3 normal1 = getBump(uvNormal1);

    writeOutput(VAR._texCoord, normalize(VAR._tbn * normalize(normal0 + normal1)));
#else

    const vec3 normalWV = getNormal(VAR._texCoord);
    vec3 uvReflection = ((VAR._vertexWVP.xyz / VAR._vertexWVP.w) + 1.0f) * 0.5f;
    uvReflection = clamp(uvReflection, vec3(0.001f), vec3(0.999f));

    vec2 uvFinalReflect = uvReflection.xy;
    vec2 uvFinalRefract = uvReflection.xy;

    //uvFinalReflect += (_noiseFactor * normalWV.xy);
    //uvFinalRefract += (_noiseFactor * normalWV.xy);

    //vec4 distOffset = texture(texDiffuse0, VAR._texCoord + vec2(time2)) * kDistortion;
    //vec4 dudvColor = texture(texDiffuse0, vec2(VAR._texCoord + distOffset.xy));
    //dudvColor = normalize(dudvColor * 2.0 - 1.0) * kRefraction;

    //normalWV = texture(texNormalMap, vec2(VAR._texCoord + dudvColor.xy)).rgb;

    const mat4 colourMatrix = dvd_Matrices[DATA_IDX]._colourMatrix;
    const float metalness = Metallic(colourMatrix);

    const vec4 refractionColour = texture(texRefractPlanar, uvFinalReflect);
    const vec4 reflectionColour = texture(texReflectPlanar, uvFinalReflect);
    
    const vec3 normalW = (dvd_InverseViewMatrix * vec4(VAR._normalWV, 0.0f)).xyz;
    const vec4 texColour = mix(mix(refractionColour, reflectionColour, Fresnel(_incident, normalW, metalness)),
                                   refractionColour,
                                   _underwater);

    _private_reflect = texColour.rgb;
    writeOutput(getPixelColour(vec4(texColour.rgb, 1.0f), colourMatrix, normalWV, VAR._texCoord));
#endif
}
