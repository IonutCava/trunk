-- Vertex

#include "vbInputData.vert"
#include "lightingDefaults.vert"

layout(location = 0) out flat int _underwater;
layout(location = 1) out vec4 _vertexWVP;

void main(void)
{
    const NodeTransformData data = fetchInputData();
    _vertexWVP = computeData(data);
    setClipPlanes();
    computeLightVectors(data);

    _underwater = dvd_cameraPosition.y < VAR._vertexW.y ? 1 : 0;

    gl_Position = _vertexWVP;
}

-- Fragment

layout(location = 0) in flat int _underwater;
layout(location = 1) in vec4 _vertexWVP;

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
#else // PRE_PASS
#define SHADOW_INTENSITY_FACTOR 0.5f
#define F0 vec3(0.02)
#include "environment.frag"
#include "BRDF.frag"
#include "output.frag"

const float Eta = 0.15f; //water

float Fresnel(in vec3 viewDir, in vec3 normal) {
    const float fresnel = Eta + (1.0f - Eta) * pow(max(0.0f, 1.0f - dot(viewDir, normal)), 5.0f);
    return saturate(fresnel);
}

vec3 _private_reflect = vec3(0.f);

vec3 ImageBasedLighting(in vec3 colour, in vec3 normalWV, in float metallic, in float roughness, in uint textureSize) {
    // This will actually return the fresnel'ed mixed between reflection and refraction as that's more useful for debugging
    if (dvd_materialDebugFlag == DEBUG_REFLECTIONS) {
        return _private_reflect;
    }

    return colour;
}

#endif // PRE_PASS

void main()
{
    NodeMaterialData data = dvd_Materials[MATERIAL_IDX];

#if defined(PRE_PASS)
#if defined(HAS_PRE_PASS_DATA)
    const float time2 = MSToSeconds(dvd_time) * 0.05f;
    vec2 uvNormal0 = VAR._texCoord * _noiseTile;
    uvNormal0.s += time2;
    uvNormal0.t += time2;
    vec2 uvNormal1 = VAR._texCoord * _noiseTile;
    uvNormal1.s -= time2;
    uvNormal1.t += time2;

    const vec3 normal0 = texture(texNormalMap, uvNormal0).rgb;
    const vec3 normal1 = texture(texNormalMap, uvNormal1).rgb;
    const vec3 normal = normalize(2.0f * ((normal0 + normal1) * 0.5f) - 1.0f);
    //vec3 normal = normalPartialDerivativesBlend(normal0, normal1);

    writeOutput(data,  VAR._texCoord, getTBNWV() * normal);
#endif //HAS_PRE_PASS_DATA
#else //PRE_PASS
    const vec3 incident = normalize(dvd_cameraPosition.xyz - VAR._vertexW.xyz);
    const vec3 normalWV = getNormalWV(VAR._texCoord);

    const vec2 waterUV = clamp(0.5f * homogenize(_vertexWVP).xy + 0.5f, vec2(0.001f), vec2(0.999f));
    const vec3 refractionColour = texture(texRefractPlanar, waterUV).rgb + _refractionTint;

    vec3 texColour;
    if (_underwater == 1) {
        texColour = refractionColour;
    } else {
        _private_reflect = texture(texReflectPlanar, waterUV).rgb;
        texColour = mix(refractionColour, _private_reflect, Fresnel(incident, mat3(dvd_InverseViewMatrix) * normalWV));
    }

    vec4 outColour = getPixelColour(vec4(texColour, 1.0f), data, normalWV, VAR._texCoord);

    // Calculate the reflection vector using the normal and the direction of the light.
    vec3 reflection = -reflect(-dvd_sunDirection.xyz, mat3(dvd_InverseViewMatrix) * normalWV);
    // Calculate the specular light based on the reflection and the camera position.
    float specular = dot(normalize(reflection), incident);
    // Check to make sure the specular was positive so we aren't adding black spots to the water.
    if (specular > 0.0f) 
    {
        // Increase the specular light by the shininess value.
        specular = pow(specular, _specularShininess);

        // Add the specular to the final color.
        //outColour = saturate(outColour + specular);
        outColour = outColour + specular;
    }

    writeOutput(outColour);
#endif
}
