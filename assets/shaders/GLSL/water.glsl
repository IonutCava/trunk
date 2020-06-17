-- Vertex

#include "vbInputData.vert"
#include "lightingDefaults.vert"

layout(location = 0) out flat int _underwater;
layout(location = 1) out vec3 _incident;
layout(location = 2) out vec3 _normalW;

void main(void)
{
    const NodeData data = fetchInputData();
    computeData(data);
    setClipPlanes();
    computeLightVectors(data);

    _underwater = dvd_cameraPosition.y < VAR._vertexW.y ? 1 : 0;
    _normalW = mat3(dvd_InverseViewMatrix) * VAR._normalWV;
    _incident = normalize(dvd_cameraPosition.xyz - VAR._vertexW.xyz);

    gl_Position = VAR._vertexWVP;
}

-- Fragment

#define SHADOW_INTENSITY_FACTOR 0.5f

#define F0 vec3(0.02)

layout(location = 0) in flat int _underwater;
layout(location = 1) in vec3 _incident;
layout(location = 2) in vec3 _normalW;

uniform vec2 _noiseTile;
uniform vec2 _noiseFactor;
uniform vec3 _refractionTint;
uniform float _specularShininess = 200.0f;

#define CUSTOM_IBL
#define USE_SHADING_BLINN_PHONG
#define USE_PLANAR_REFLECTION
#define USE_PLANAR_REFRACTION

#if defined(PRE_PASS)
#include "prePass.frag"
#include "utility.frag"
#else // PRE_PASS
#include "BRDF.frag"
#include "output.frag"

const float Eta = 0.15f; //water

float Fresnel(in vec3 viewDir, in vec3 normal) {
    const float fresnel = Eta + (1.0f - Eta) * pow(max(0.0f, 1.0f - dot(viewDir, normal)), 5.0f);
    return saturate(fresnel);
}

vec3 _private_reflect = vec3(0.f);

vec3 ImageBasedLighting(in vec3 colour, in vec3 normalWV, in float metallic, in float roughness, in int textureSize) {
    // This will actually return the fresnel'ed mixed between reflection and refraction as that's more useful for debugging
    return _private_reflect;
}
#endif // PRE_PASS

void main()
{
    NodeData data = dvd_Matrices[DATA_IDX];
    prepareData(data);

#if defined(PRE_PASS)
#if defined(HAS_PRE_PASS_DATA)
    const float time2 = MSToSeconds(dvd_time) * 0.05f;
    vec2 uvNormal0 = VAR._texCoord * _noiseTile;
    uvNormal0.s += time2;
    uvNormal0.t += time2;
    vec2 uvNormal1 = VAR._texCoord * _noiseTile;
    uvNormal1.s -= time2;
    uvNormal1.t += time2;

    vec3 normal0 = (2.0f * texture(texNormalMap, uvNormal0).rgb - 1.0f);
    vec3 normal1 = (2.0f * texture(texNormalMap, uvNormal1).rgb - 1.0f);
    vec3 normal = normalize((normal0 + normal1)/* * 0.5f*/);

    writeOutput(data, 
                VAR._texCoord,
                getTBNWV() * normal);
#endif //HAS_PRE_PASS_DATA
#else

    const float kDistortion = 0.015f;
    const float kRefraction = 0.09f;

    const vec3 normalWV = getNormalWV(VAR._texCoord);
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

    const vec4 refractionColour = texture(texRefractPlanar, uvFinalReflect) + vec4(_refractionTint, 1.0f);
    const vec4 reflectionColour = texture(texReflectPlanar, uvFinalReflect);
    
    const vec3 incident = normalize(dvd_cameraPosition.xyz - VAR._vertexW.xyz);
    const vec3 normalW = mat3(dvd_InverseViewMatrix) * VAR._normalWV;

    //const vec3 incident = normalize(_incident);
    //const vec3 normalW = normalize(_normalW);

    const vec4 texColour = mix(mix(refractionColour, reflectionColour, Fresnel(incident, normalW)),
                                   refractionColour,
                                   _underwater);
    _private_reflect = texColour.rgb;
    vec4 outColour = getPixelColour(vec4(texColour.rgb, 1.0f), data, normalWV, VAR._texCoord);

    const uint dirLightCount = dvd_LightData.x;
    vec3 ret = vec3(0.0);
    for (uint lightIdx = 0; lightIdx < dirLightCount; ++lightIdx) {
        const Light light = dvd_LightSource[lightIdx];
        const vec3 lightDirectionWV = -light._directionWV.xyz;
        // Calculate the reflection vector using the normal and the direction of the light.
        vec3 reflection = -reflect(normalize(lightDirectionWV), normalWV);
        // Calculate the specular light based on the reflection and the camera position.
        float specular = dot(normalize(reflection), normalize(VAR._viewDirectionWV));
        // Check to make sure the specular was positive so we aren't adding black spots to the water.
        if (specular > 0.0f) {
            // Increase the specular light by the shininess value.
            specular = pow(specular, _specularShininess);

            // Add the specular to the final color.
            outColour = saturate(outColour + specular);
        }
    }

    writeOutput(outColour);
#endif
}
