-- Vertex

#include "vbInputData.vert"
#include "lightingDefaults.vert"

out flat int _underwater;
out vec3 _pixToEye;
out vec4 _vertexWVP;

void main(void)
{
    computeData();
    
    computeLightVectors();

    _pixToEye  = -VAR._vertexWV.xyz;
    _vertexWVP = dvd_ViewProjectionMatrix * VAR._vertexW;
    _underwater = dvd_cameraPosition.y < VAR._vertexW.y ? 1 : 0;

    gl_Position = _vertexWVP;
}

-- Fragment

in flat int _underwater;
in vec3 _pixToEye;
in vec4 _vertexWVP;

layout(location = 0) out vec4 _colourOut;
layout(location = 1) out vec2 _normalOut;
layout(location = 2) out vec2 _velocityOut;

uniform vec2 _noiseTile;
uniform vec2 _noiseFactor;
uniform float _waterShininess;

#include "lightInput.cmn"
#include "materialData.frag"
#include "shadowMapping.frag"
#include "velocityCalc.frag"

float Fresnel(in vec3 viewDir, in vec3 normal) {
    return _underwater == 1 ? 1.0 : 1.0 / pow(1.0 + dot(viewDir, normal), 5);
}

void main (void)
{  
#   define texWaterReflection texReflectPlanar
#   define texWaterRefraction texRefractPlanar
#   define texWaterNoiseNM texDiffuse0
#   define texWaterNoiseDUDV texDiffuse1

    dvd_private_light = dvd_LightSource[0];
    float time2 = float(dvd_time) * 0.00001;
    const float kDistortion = 1.0;// 0.015;
    vec4 distOffset = texture(texWaterNoiseDUDV, VAR._texCoord + vec2(time2)) * kDistortion;

    vec4 uvReflection = _vertexWVP / _vertexWVP.w;
    uvReflection += vec4(1.0);
    uvReflection *= vec4(0.5);
    uvReflection = clamp(uvReflection, vec4(0.001), vec4(0.999));

    vec3 normal = texture(texWaterNoiseNM, vec2(VAR._texCoord + distOffset.xy)).rgb;
    normal = normalize(normal * 2.0 - 1.0);

    vec2 uvFinalReflect = uvReflection.xy;// +_noiseFactor * normal.xy;
    vec2 uvFinalRefract = uvReflection.xy;// +_noiseFactor * normal.xy;

    vec3 N = normalize(dvd_NormalMatrixWV(VAR.dvd_instanceID) * normal);
    vec3 L = getLightDirection();
    vec3 V = normalize(_pixToEye);

    float iSpecular = pow(clamp(dot(normalize(reflect(-L, N)), V), 0.0, 1.0), _waterShininess);

    // add Diffuse
    _colourOut.rgb = mix(texture(texWaterReflection, uvFinalReflect).rgb,
                         texture(texWaterRefraction, uvFinalRefract).rgb,
                         vec3(clamp(Fresnel(V, normalize(VAR._normalWV)), 0.0, 1.0)));

    // add Specular
    _colourOut.rgb = texture(texWaterRefraction, uvFinalRefract).rgb * 10.2;//clamp(_colourOut.rgb + dvd_private_light._colour.rgb * getSpecular() * iSpecular, vec3(0.0), vec3(1.0));
    _normalOut = packNormal(N);
    _velocityOut = velocityCalc(dvd_InvProjectionMatrix, getScreenPositionNormalised());
}
