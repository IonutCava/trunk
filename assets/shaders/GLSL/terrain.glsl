-- Vertex.Depth
uniform mat4 dvd_WorldMatrix;//[MAX_INSTANCES];

layout(std140) uniform dvd_MatrixBlock
{
    mat4 dvd_ProjectionMatrix;
    mat4 dvd_ViewMatrix;
    mat4 dvd_ViewProjectionMatrix;
};

void main(void){
    vec4 dvd_Vertex = vec4(inVertexData,1.0);
    gl_Position = dvd_ViewProjectionMatrix * dvd_WorldMatrix * dvd_Vertex;
}

-- Vertex
//based on: http://yannick.gerometta.free.fr/base.php?id=glsldemo
#include "vbInputData.vert"

uniform vec3 bbox_min;
uniform vec3 bbox_diff;

#include "lightingDefaults.vert"

void main(void){

    computeData();

    _texCoord = vec3((dvd_Vertex.xyz - bbox_min) / bbox_diff).sp;

    computeLightVectors();
    
    gl_Position = dvd_ViewProjectionMatrix * dvd_Vertex;
}

-- Fragment.Depth

out vec4 _colorOut;

#include "lightingDefaults.frag"

void main (void)
{
    computeData();
    _colorOut = vec4(1.0,1.0,1.0,1.0);
}

-- Fragment

//based on: http://yannick.gerometta.free.fr/base.php?id=glsldemo
in vec2 _texCoord;
in vec4 _vertexW;

#include "lightInput.cmn"

out vec4 _colorOut;

uniform sampler2D texDiffuseMap;
uniform sampler2D texNormalHeightMap;
uniform sampler2D texBlend0;
uniform sampler2D texBlend1;
uniform sampler2D texBlend2;
uniform sampler2D texBlend3;
uniform sampler2D texWaterCaustics;

uniform int   dvd_lightCount;
uniform float dvd_time;
uniform bool  dvd_isReflection;

uniform float detail_scale;
uniform float diffuse_scale;
uniform float _waterHeight;
uniform mat4 material;
uniform vec3 bbox_min;
uniform vec4 dvd_lightAmbient;

#define LIGHT_DIRECTIONAL		0
#define LIGHT_OMNIDIRECTIONAL	1
#define LIGHT_SPOT				2

vec4 NormalMapping(in vec2 uv,in vec3 pixelToLightTBN);
vec4 NormalMappingUnderwater(in vec2 uv,in vec3 pixelToLightTBN);
vec4 CausticsColor();

#include "lightingDefaults.frag"
#include "shadowMapping.frag"


bool isUnderWater() { return gl_ClipDistance[0] < 0; }
    
void main (void)
{
    if(isUnderWater()){
        _colorOut = NormalMappingUnderwater(_texCoord, _lightDirection[0]);
    }else{
        _colorOut = NormalMapping(_texCoord, _lightDirection[0]);
    }
    applyFog(_colorOut);
}

vec4 CausticsColor()
{
    float time2 = dvd_time * 0.0001;
    vec2 noiseUV = _texCoord*100.0;
    vec2 uv0 = noiseUV;
    vec2 uv1 = noiseUV + time2;
    uv0.t = uv0.t;
    uv0.s -= time2;
    
    return (texture(texWaterCaustics, uv1) + texture(texWaterCaustics, uv0)) * 0.5;	
}

const float shadowMaxDistance = 200.0;
vec4 NormalMapping(in vec2 uv, in vec3 pixelToLightTBN)
{	
    if (dvd_isReflection) computeData();
    vec3 lightVecTBN = normalize(pixelToLightTBN);
    vec3 viewVecTBN  = normalize(_viewDirection);
    vec2 uv_detail   = uv * detail_scale;
    vec2 uv_diffuse  = uv * diffuse_scale;
    
    vec3 normalTBN = normalize(texture(texNormalHeightMap, uv_detail).rgb * 2.0 - 1.0);

    vec4 tBase[3];
    tBase[0] = texture(texBlend0, uv_diffuse);
    tBase[1] = texture(texBlend1, uv_diffuse);	
    tBase[2] = texture(texBlend2, uv_diffuse);

    vec4 DiffuseMap = texture(texDiffuseMap, uv);

    vec4 cBase = mix(mix(tBase[1], tBase[0], DiffuseMap.r), tBase[2], DiffuseMap.g);	
        
#if defined(USE_ALPHA_TEXTURE)
        vec4 tBase3 = texture(texBlend3, uv_diffuse);
        cBase = mix(cBase, tBase3, DiffuseMap.b);
#endif

    float iDiffuse = max(dot(lightVecTBN.xyz, normalTBN), 0.0);	// diffuse intensity. NDotL

    vec4 cAmbient = dvd_lightAmbient * material[0];
    vec4 cDiffuse = material[1] * iDiffuse;

    Light currentLight = dvd_LightSource[dvd_lightIndex[0]];

    if(dvd_lightCount > 0){
        cAmbient += currentLight._diffuse.w * material[0];
        cDiffuse *= vec4(currentLight._diffuse.rgb, 1.0);
    }

    // SHADOW MAPS
    float shadow = 1.0;
    float distance = length(_viewDirection);
    if(distance < shadowMaxDistance) {
        applyShadowDirectional(0, shadow);
        shadow = 0.2 + 0.8 * (1.0 - (1.0-shadow) * (shadowMaxDistance-distance) / shadowMaxDistance);
    }

    return (cAmbient * cBase +  cDiffuse * cBase) * shadow;
}
    
vec4 NormalMappingUnderwater(in vec2 uv, in vec3 pixelToLightTBN)
{	
    if (dvd_isReflection) computeData();

    vec3 lightVecTBN = normalize(pixelToLightTBN);
    vec3 viewVecTBN  = normalize(_viewDirection);
    vec2 uv_detail   = uv * detail_scale;
    vec2 uv_diffuse  = uv * diffuse_scale;
    
    vec3 normalTBN = normalize(texture(texNormalHeightMap, uv_detail).rgb * 2.0 - 1.0);

    vec4 cBase = texture(texBlend0, uv_diffuse);

    float iDiffuse = max(dot(lightVecTBN.xyz, normalTBN), 0.0);	// diffuse intensity. NDotL
    float iSpecular = clamp(pow(max(dot(reflect(-lightVecTBN.xyz, normalTBN), viewVecTBN), 0.0), material[3].x ), 0.0, 1.0);

    vec4 cAmbient = dvd_lightAmbient * material[0];
    vec4 cDiffuse = material[1] * iDiffuse;
    vec4 cSpecular = vec4(0);
    
    float alpha = 0.0f;
    vec4 caustic = cSpecular;
    Light currentLight = dvd_LightSource[dvd_lightIndex[0]];

    if(dvd_lightCount > 0){
        cAmbient += currentLight._diffuse.w * material[0];
        cDiffuse *= vec4(currentLight._diffuse.rgb, 1.0);
        // Add specular intensity
        cSpecular = vec4(currentLight._specular.rgb, 1.0) * material[2] * iSpecular;
        alpha = (_waterHeight - _vertexW.y) / (2*(_waterHeight - bbox_min.y));
        caustic = alpha * CausticsColor();
    }

    // SHADOW MAPS
    float shadow = 1.0;
    float distance = length(_viewDirection);
    if(distance < shadowMaxDistance) {
        applyShadowDirectional(0, shadow);
        shadow = 0.2 + 0.8 * (1.0 - (1.0-shadow) * (shadowMaxDistance-distance) / shadowMaxDistance);
    }

    return ((1-alpha) * (cAmbient * cBase +  cDiffuse * cBase + cSpecular) + caustic) * shadow;
}