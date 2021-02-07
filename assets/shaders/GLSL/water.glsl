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

uniform vec3 _refractionTint;
uniform float _specularShininess;
uniform vec2 _noiseTile;
uniform vec2 _noiseFactor;

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

vec3 ImageBasedLighting(in vec3 colour, in vec3 normalWV, in float metallic, in float roughness, in uint textureSize, in uint probeIdx) {
    // This will actually return the fresnel'ed mixed between reflection and refraction as that's more useful for debugging
    if (dvd_materialDebugFlag == DEBUG_REFLECTIONS) {
        return _private_reflect;
    }

    return colour;
}

#endif // PRE_PASS

void main()
{
#if defined(PRE_PASS)
#if defined(HAS_PRE_PASS_DATA)
    const float time2 = MSToSeconds(dvd_time) * 0.05f;
    const vec2 uvNormal0 = (VAR._texCoord * _noiseTile) + vec2( time2, time2);
    const vec2 uvNormal1 = (VAR._texCoord * _noiseTile) + vec2(-time2, time2);

    const vec3 normal0 = texture(texNormalMap, uvNormal0).rgb;
    const vec3 normal1 = texture(texNormalMap, uvNormal1).rgb;
    const vec3 normal = normalize(2.0f * ((normal0 + normal1) * 0.5f) - 1.0f);
    //vec3 normal = normalPartialDerivativesBlend(normal0, normal1);

    writeGBuffer(1.0f,  VAR._texCoord, getTBNWV() * normal);
#endif //HAS_PRE_PASS_DATA
#else //PRE_PASS
    const vec3 incident = normalize(dvd_cameraPosition.xyz - VAR._vertexW.xyz);
    const vec3 normalWV = getNormalWV(VAR._texCoord);
    const vec3 normaW = normalize(mat3(dvd_InverseViewMatrix) * normalWV);

    const vec2 waterUV = clamp(0.5f * homogenize(_vertexWVP).xy + 0.5f, vec2(0.001f), vec2(0.999f));
    const vec3 refractionColour = texture(texRefractPlanar, waterUV).rgb + _refractionTint;

    vec3 texColour = refractionColour;
    if (_underwater == 0) {
        _private_reflect = texture(texReflectPlanar, waterUV).rgb;
        // Get a modified viewing direction of the camera that only takes into account height.
        // ref: https://www.rastertek.com/terdx10tut16.html
        texColour = mix(refractionColour, _private_reflect, Fresnel(incident.yyy, normaW));
    }

    NodeMaterialData data = dvd_Materials[MATERIAL_IDX];

    const vec4 outColour = getPixelColour(vec4(texColour, 1.0f), data, normalWV, VAR._texCoord, 0u);

    // Guess work based on what "look right"
    const float lerpValue = saturate(2.95f * (dvd_sunDirection.y + 0.15f));
    const vec3 lightDirection = mix(-dvd_sunDirection.xyz, dvd_sunDirection.xyz, lerpValue);

    // Calculate the reflection vector using the normal and the direction of the light.
    const vec3 reflection = -reflect(lightDirection, normaW);
    // Calculate the specular light based on the reflection and the camera position.
    const float specular = dot(normalize(reflection), incident);
    // Increase the specular light by the shininess value and add the specular to the final color.
    // Check to make sure the specular was positive so we aren't adding black spots to the water.
    writeScreenColour(outColour + (specular > 0.f ? pow(specular, _specularShininess) : 0.f));
#endif
}
