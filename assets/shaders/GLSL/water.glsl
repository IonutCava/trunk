-- Vertex

#include "vbInputData.vert"
#include "lightingDefaults.vert"

out flat int _underwater;
out vec4 _vertexWVP;

void main(void)
{
    computeData();
    
    computeLightVectors();

    _vertexWVP = dvd_ProjectionMatrix * VAR._vertexWV;
    _underwater = dvd_cameraPosition.y < VAR._vertexW.y ? 1 : 0;

    gl_Position = _vertexWVP;
}

-- Fragment

#define SHADOW_INTENSITY_FACTOR 0.5f

in flat int _underwater;
in vec4 _vertexWVP;

uniform vec2 _noiseTile;
uniform vec2 _noiseFactor;

#define USE_SHADING_BLINN_PHONG

#include "BRDF.frag"
#include "shadowMapping.frag"

#if defined(PRE_PASS)
#include "prePass.frag"
#else
#include "output.frag"
#endif

const float Eta = 0.15f; //water

float Fresnel(in vec3 viewDir, in vec3 normal) {
    if (_underwater == 1) {
        return 1.0;
    }
    
    return Eta + (1.0 - Eta) * pow(max(0.0f, 1.0f - dot(viewDir, normal)), 5.0f);
}

void main()
{  
    const float kDistortion = 0.015f;
    const float kRefraction = 0.09f;

    vec4 uvReflection = clamp(((_vertexWVP / _vertexWVP.w) + 1.0f) * 0.5f, vec4(0.001f), vec4(0.999f));

    float time2 = float(dvd_time) * 0.00001f;
    vec2 uvNormal0 = VAR._texCoord * _noiseTile;
    uvNormal0.s += time2;
    uvNormal0.t += time2;
    vec2 uvNormal1 = VAR._texCoord * _noiseTile;
    uvNormal1.s -= time2;
    uvNormal1.t += time2;

    vec3 normal0 = texture(texNormalMap, uvNormal0).rgb * 2.0f - 1.0f;
    vec3 normal1 = texture(texNormalMap, uvNormal1).rgb * 2.0f - 1.0f;
    vec3 normal = normalize(normal0 + normal1);

    vec3 incident = normalize(-VAR._vertexWV.xyz);

    vec2 uvFinalReflect = uvReflection.xy + _noiseFactor * normal.xy;
    vec2 uvFinalRefract = uvReflection.xy + _noiseFactor * normal.xy;

    /*vec4 distOffset = texture(texDiffuse0, VAR._texCoord + vec2(time2)) * kDistortion;
    vec4 dudvColor = texture(texDiffuse0, vec2(VAR._texCoord + distOffset.xy));
    dudvColor = normalize(dudvColor * 2.0 - 1.0) * kRefraction;

    normal = texture(texNormalMap, vec2(VAR._texCoord + dudvColor.xy)).rgb;*/
    normal = normalize(normal * 2.0f - 1.0f);

    normal = normalize(getTBNMatrix() * normal);

    mat4 colourMatrix = dvd_Matrices[VAR.dvd_baseInstance]._colourMatrix;

#if defined(PRE_PASS)
    outputWithVelocity(normal);
#else

    vec4 mixFactor = vec4(clamp(Fresnel(incident, normalize(VAR._normalWV)), 0.0f, 1.0f));
    vec4 texColour = mix(texture(texReflectPlanar, uvFinalReflect),
        texture(texRefractPlanar, uvFinalRefract),
        mixFactor);

    writeOutput(getPixelColour(texColour, colourMatrix, normal));
#endif
}