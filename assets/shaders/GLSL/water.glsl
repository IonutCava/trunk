-- Vertex

#include "vbInputData.vert"
#include "lightInput.cmn"

out vec3 _pixToEye;
out vec4 _vertexWVP;

void main(void)
{
    computeData();

    _pixToEye   = -vec3(dvd_ViewMatrix * _vertexW);

    _vertexWVP = dvd_ViewProjectionMatrix * _vertexW;
    _normalWV = normalize(dvd_NormalMatrix * dvd_Normal);

    gl_Position = _vertexWVP;
}

-- Fragment

in vec4 _vertexWVP;

in vec3 _pixToEye;

out vec4 _colorOut;

uniform vec2 _noiseTile;
uniform vec2 _noiseFactor;
uniform float _waterShininess;
uniform float _transparencyBias;
uniform bool  underwater;

uniform sampler2D texWaterNoiseNM;
uniform sampler2D texWaterNoiseDUDV;
uniform sampler2D texWaterReflection;
uniform sampler2D texWaterRefraction;

uniform float dvd_time;
uniform ivec2 dvd_invScreenDimension;

#include "lightInput.cmn"
#include "lightingDefaults.frag"
#include "shadowMapping.frag"

float Fresnel(in vec3 viewDir, in vec3 normal) {
    return underwater ? 1.0 : 1.0 / pow(1.0 + dot(viewDir, normal), 5);
}

void main (void)
{  
    vec4 uvReflection = _vertexWVP * vec4(1.0 / _vertexWVP.w);
    uvReflection += vec4(1.0);
    uvReflection *= vec4(0.5);
    uvReflection = clamp(uvReflection, vec4(0.001), vec4(0.999));

    float time2 = dvd_time * 0.00001;
    vec2 uvNormal0 = _texCoord * _noiseTile;
    uvNormal0.s += time2;
    uvNormal0.t += time2;
    vec2 uvNormal1 = _texCoord * _noiseTile;
    uvNormal1.s -= time2;
    uvNormal1.t += time2;

    vec3 normal0 = texture(texWaterNoiseNM, uvNormal0).rgb * 2.0 - 1.0;
    vec3 normal1 = texture(texWaterNoiseNM, uvNormal1).rgb * 2.0 - 1.0;
    vec3 normal = normalize(normal0 + normal1);

    vec2 uvFinalReflect = uvReflection.xy + _noiseFactor * normal.xy;
    vec2 uvFinalRefract = uvReflection.xy + _noiseFactor * normal.xy;

    vec3 N = normalize(dvd_NormalMatrix * normal);
    vec3 L = normalize(-(dvd_LightSource[0]._position.xyz));
    vec3 V = normalize(_pixToEye);

    float iSpecular = pow(clamp(dot(normalize(reflect(-L, N)), V), 0.0, 1.0), _waterShininess);

    // add Diffuse
    _colorOut.rgb = mix(texture(texWaterReflection, uvFinalRefract).rgb, 
                        texture(texWaterRefraction, uvFinalReflect).rgb, 
                        vec3(clamp(Fresnel(V, normalize(_normalWV)), 0.0, 1.0)));
    // add Specular
    _colorOut.rgb = clamp(_colorOut.rgb + dvd_LightSource[0]._color.rgb * dvd_MatSpecular.rgb * iSpecular, vec3(0.0), vec3(1.0));
    // shadow mapping
    if (!underwater){
        _colorOut.rgb *= (0.2 + 0.8 * applyShadowDirectional(0, dvd_ShadowSource[0]));
    }
    // add Fog
    _colorOut = applyFog(vec4(_colorOut.rgb, 1.0));
}
