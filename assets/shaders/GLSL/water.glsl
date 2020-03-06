-- Vertex

#include "vbInputData.vert"
#include "lightingDefaults.vert"

layout(location = 0) out flat int _underwater;
layout(location = 1) out vec4 _vertexWVP;

void main(void)
{
    computeData();
    
    computeLightVectors(dvd_NormalMatrixWV(DATA_IDX));

    _vertexWVP = dvd_ProjectionMatrix * VAR._vertexWV;
    _underwater = dvd_cameraPosition.y < VAR._vertexW.y ? 1 : 0;

    gl_Position = _vertexWVP;
}

-- Fragment

#define SHADOW_INTENSITY_FACTOR 0.5f

layout(location = 0) in flat int _underwater;
layout(location = 1) in vec4 _vertexWVP;

uniform vec2 _noiseTile;
uniform vec2 _noiseFactor;

#define USE_SHADING_BLINN_PHONG
#define USE_DEFERRED_NORMALS
#define USE_PLANAR_REFLECTION
#define USE_PLANAR_REFRACTION

#if defined(PRE_PASS)
#include "prePass.frag"
#else
#include "BRDF.frag"
#include "output.frag"

const float Eta = 0.15f; //water

float Fresnel(in vec3 viewDir, in vec3 normal) {
    return Eta + (1.0 - Eta) * pow(max(0.0f, 1.0f - dot(viewDir, normal)), 5.0f);
}
#endif

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

    vec3 normalWV = getNormal(VAR._texCoord);
    vec3 uvReflection = clamp(((_vertexWVP.xyz / _vertexWVP.w) + 1.0f) * 0.5f, vec3(0.001f), vec3(0.999f));
    vec3 incident = normalize(-VAR._vertexWV.xyz);

    vec2 uvFinalReflect = uvReflection.xy;
    vec2 uvFinalRefract = uvReflection.xy;

    //uvFinalReflect += (_noiseFactor * normalWV.xy);
    //uvFinalRefract += (_noiseFactor * normalWV.xy);

    //vec4 distOffset = texture(texDiffuse0, VAR._texCoord + vec2(time2)) * kDistortion;
    //vec4 dudvColor = texture(texDiffuse0, vec2(VAR._texCoord + distOffset.xy));
    //dudvColor = normalize(dudvColor * 2.0 - 1.0) * kRefraction;

    //normalWV = texture(texNormalMap, vec2(VAR._texCoord + dudvColor.xy)).rgb;

    mat4 colourMatrix = dvd_Matrices[DATA_IDX]._colourMatrix;
    vec4 refractionColour = texture(texRefractPlanar, uvFinalReflect);

    vec4 texColour = mix(mix(refractionColour, texture(texReflectPlanar, uvFinalRefract), saturate(Fresnel(incident, VAR._normalWV))),
                         refractionColour,
                         _underwater);


    writeOutput(getPixelColour(vec4(texColour.rgb, 1.0f), colourMatrix, normalWV, VAR._texCoord));
    
    //writeOutput(vec4(texture(texReflectPlanar, uvFinalReflect).rgb, 1.0f));
    //writeOutput(vec4(VAR._normalWV, 1.0f));
    //writeOutput(texColour);
#endif
}