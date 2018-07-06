//based on: http://yannick.gerometta.free.fr/base.php?id=glsldemo
-- Vertex
#include "vbInputData.vert"
#include "lightingDefaults.vert"

uniform vec3 bbox_min;
uniform vec3 bbox_extent;

void main(void){

    computeData();

    _texCoord = vec3((_vertexW.xyz - bbox_min) / bbox_extent).sp;
    computeLightVectors();
    gl_Position = dvd_ViewProjectionMatrix * _vertexW;
}

-- Fragment

//based on: http://yannick.gerometta.free.fr/base.php?id=glsldemo
in vec2 _texCoord;
in vec4 _vertexW;
uniform int  lodLevel = 0;
#include "lightInput.cmn"

out vec4 _colorOut;

uniform float dvd_time;
uniform float dvd_waterHeight;

uniform mat4 material;
uniform vec3 bbox_min;

#define LIGHT_DIRECTIONAL		0
#define LIGHT_OMNIDIRECTIONAL	1
#define LIGHT_SPOT				2

#include "lightingDefaults.frag"
#include "shadowMapping.frag"
#include "terrainSplatting.frag"

bool isUnderwater() { return gl_ClipDistance[0] <= 0.0; }

vec3 lightVecTBN;
vec4 currentLightDiffuse;
vec4 currentLightSpecular;

vec4 CausticsColor()
{
    float time2 = dvd_time * 0.0001;
    vec2 noiseUV = _texCoord * underwaterDiffuseScale;
    vec2 uv0 = noiseUV;
    vec2 uv1 = noiseUV + time2;
    uv0.s -= time2;
    
    return (texture(texWaterCaustics, uv1) + texture(texWaterCaustics, uv0)) * 0.5;	
}

vec4 DiffuseColor(in vec3 normalTBN, in vec4 textureColor){
    return (material[1] * vec4(currentLightDiffuse.rgb, 1.0) * textureColor) * max(dot(lightVecTBN, normalTBN), 0.0);
}

vec4 AmbientColor(in vec4 textureColor){
    return (dvd_lightAmbient * material[0]) + (currentLightDiffuse.w * material[0]) * textureColor;
}

vec4 SpecularColor(in vec3 normalTBN){
    return vec4(currentLightSpecular.rgb, 1.0) * material[2] * clamp(pow(max(dot(reflect(-lightVecTBN, normalTBN), normalize(_viewDirection)), 0.0), material[3].x), 0.0, 1.0);
}

vec4 NormalMapping(in float shadow) {	
    vec3 normalTBN;
    vec4 cBase;
    getColorAndTBNNormal(cBase, normalTBN);

    vec4 cDiffuse = DiffuseColor(normalTBN, cBase);
    vec4 cAmbient = AmbientColor(cBase);

    return (cAmbient + cDiffuse) * shadow;
}

vec4 NormalMappingUnderwater(in float shadow)
{	
    vec3 normalTBN;
    vec4 cBase;
    getColorAndTBNUnderwater(cBase, normalTBN);

    vec4 cDiffuse  = DiffuseColor(normalTBN, cBase);
    vec4 cAmbient  = AmbientColor(cBase);
    vec4 cSpecular = SpecularColor(normalTBN);
    
    // Add specular intensity
    float alpha = (dvd_waterHeight - _vertexW.y) / ( 2 * (dvd_waterHeight - bbox_min.y));

    return mix(((cAmbient + cDiffuse) * shadow + cSpecular), CausticsColor(), alpha);
}

void FinalColor(inout vec4 linearColor){
    // Gama correction
    vec3 gamma = vec3(1.0/2.2);
    //	linearColor.rgb = pow(linearColor.rgb, gamma);
#if defined(_DEBUG)
    if (dvd_showShadowSplits){
        if (_shadowTempInt == -1)
            linearColor = vec4(1.0);
        if (_shadowTempInt == 0)
            linearColor.r += 0.15;
        if (_shadowTempInt == 1)
            linearColor.g += 0.25;
        if (_shadowTempInt == 2)
            linearColor.b += 0.40;
        if (_shadowTempInt == 3)
            linearColor.rgb += vec3(0.15, 0.25, 0.40);
    }
#endif

    applyFog(linearColor);
}

void main(void)
{
    Light currentLight = dvd_LightSource[dvd_lightIndex[0]];
    lightVecTBN = normalize(_lightInfo[0]._lightDirection);
    currentLightDiffuse = currentLight._diffuse;
    currentLightSpecular = currentLight._specular;
    float shadow = 1.0;
    applyShadowDirectional(0, shadow);
    shadow = 0.3 + 0.7 * shadow;
    _colorOut = isUnderwater() ? NormalMappingUnderwater(shadow) : NormalMapping(shadow);

    FinalColor(_colorOut);

}
