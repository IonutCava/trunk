-- Vertex.Depth
uniform mat4 dvd_ModelMatrix;
uniform mat4 dvd_ModelViewProjectionMatrix;

in vec3  inVertexData;
out vec4 _vertexM;

void main(void){
    vec4 dvd_Vertex = vec4(inVertexData,1.0);
    _vertexM = dvd_ModelMatrix * dvd_Vertex;
    gl_Position = dvd_ModelViewProjectionMatrix * dvd_Vertex;
}

-- Vertex
//based on: http://yannick.gerometta.free.fr/base.php?id=glsldemo
#include "vboInputData.vert"

uniform vec3 bbox_min;
uniform vec3 bbox_diff;

#include "lightingDefaults.vert"

void main(void){

    computeData();

    _texCoord = vec3((dvd_Vertex.xyz - bbox_min.xyz) / bbox_diff).sp;

    _vertexMV = dvd_ViewMatrix * _vertexM;
    computeLightVectors();
    
    if(dvd_enableShadowMapping) {
        // Transformed position 
        //vec4 pos = dvd_ModelViewMatrix * dvd_Vertex;
        // position multiplied by the inverse of the camera matrix
        //pos = dvd_ModelViewMatrixInverse * pos;
        // position multiplied by the light matrix. The vertex's position from the light's perspective
        _shadowCoord[0] = dvd_lightProjectionMatrices[0] * dvd_Vertex;
    }

    gl_Position = dvd_ModelViewProjectionMatrix * dvd_Vertex;
}

-- Fragment.Depth

in vec4  _vertexM;
out vec4 _colorOut;

uniform float _waterHeight;
#if defined(SKIP_HARDWARE_CLIPPING)
in float dvd_ClipDistance[MAX_CLIP_PLANES];
#else
in float gl_ClipDistance[MAX_CLIP_PLANES];
#endif

void main (void)
{
#if defined(SKIP_HARDWARE_CLIPPING)
    if(dvd_ClipDistance[0] < 0) discard;
#else
    if(gl_ClipDistance[0] < 0) discard;
#endif
    _colorOut = vec4(1.0,1.0,1.0,1.0);
}

-- Fragment

//based on: http://yannick.gerometta.free.fr/base.php?id=glsldemo
in vec3 _lightDirection[MAX_LIGHT_COUNT];
in vec3 _viewDirection;
in vec2 _texCoord;
in vec3 _normalMV;
in vec4 _vertexM;
in vec4 _vertexMV;

#if defined(SKIP_HARDWARE_CLIPPING)
in float dvd_ClipDistance[MAX_CLIP_PLANES];
uniform int  dvd_clip_plane_active[MAX_CLIP_PLANES];
#else
in float gl_ClipDistance[MAX_CLIP_PLANES];
#endif

out vec4 _colorOut;

uniform sampler2D texDiffuseMap;
uniform sampler2D texNormalHeightMap;
uniform sampler2D texBlend0;
uniform sampler2D texBlend1;
uniform sampler2D texBlend2;
uniform sampler2D texBlend3;
uniform sampler2D texWaterCaustics;

uniform int   dvd_lightType[MAX_LIGHT_COUNT];
uniform bool  dvd_lightEnabled[MAX_LIGHT_COUNT];
uniform float dvd_time;

uniform float LODFactor;
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

///Global NDotL, basically
float iDiffuse;

#include "fog.frag"
#include "shadowMapping.frag"


#if defined(SKIP_HARDWARE_CLIPPING)
bool isUnderWater() { return dvd_ClipDistance[0] < 0; }
#else
bool isUnderWater() { return gl_ClipDistance[0] < 0; }
#endif
    
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

    iDiffuse = max(dot(lightVecTBN.xyz, normalTBN), 0.0);	// diffuse intensity. NDotL

    vec4 cAmbient = dvd_lightAmbient * material[0];
    vec4 cDiffuse = material[1] * iDiffuse;

    if(dvd_lightEnabled[0]){
        cAmbient += gl_LightSource[0].ambient * material[0];
        cDiffuse *= gl_LightSource[0].diffuse;
    }

    // SHADOW MAPS
    float shadow = 1.0;
    float distance = length(_viewDirection);
    if(distance < shadowMaxDistance) {
        applyShadowDirectional(iDiffuse, 0, shadow);
        shadow = 0.2 + 0.8 * (1.0 - (1.0-shadow) * (shadowMaxDistance-distance) / shadowMaxDistance);
    }

    return (cAmbient * cBase +  cDiffuse * cBase) * shadow;
}
    
vec4 NormalMappingUnderwater(in vec2 uv, in vec3 pixelToLightTBN)
{	
#if defined(SKIP_HARDWARE_CLIPPING)
    if(dvd_clip_plane_active[0] == 1) discard;
#else
    if(gl_ClipDistance[0] == 1) discard;
#endif
    

    vec3 lightVecTBN = normalize(pixelToLightTBN);
    vec3 viewVecTBN  = normalize(_viewDirection);
    vec2 uv_detail   = uv * detail_scale;
    vec2 uv_diffuse  = uv * diffuse_scale;
    
    vec3 normalTBN = normalize(texture(texNormalHeightMap, uv_detail).rgb * 2.0 - 1.0);

    vec4 cBase = texture(texBlend0, uv_diffuse);

    iDiffuse = max(dot(lightVecTBN.xyz, normalTBN), 0.0);	// diffuse intensity. NDotL
    float iSpecular = clamp(pow(max(dot(reflect(-lightVecTBN.xyz, normalTBN), viewVecTBN), 0.0), material[3].x ), 0.0, 1.0);

    vec4 cAmbient = dvd_lightAmbient * material[0];
    vec4 cDiffuse = material[1] * iDiffuse;
    vec4 cSpecular = vec4(0);
    
    float alpha = 0.0f;
    vec4 caustic = cSpecular;

    if(dvd_lightEnabled[0]){
        cAmbient += gl_LightSource[0].ambient * material[0];
        cDiffuse *= gl_LightSource[0].diffuse;
        // Add specular intensity
        cSpecular = gl_LightSource[0].specular * material[2] * iSpecular;
        alpha = (_waterHeight - _vertexM.y) / (2*(_waterHeight - bbox_min.y));
        caustic = alpha * CausticsColor();
    }

    // SHADOW MAPS
    float shadow = 1.0;
    float distance = length(_viewDirection);
    if(distance < shadowMaxDistance) {
        applyShadowDirectional(iDiffuse, 0, shadow);
        shadow = 0.2 + 0.8 * (1.0 - (1.0-shadow) * (shadowMaxDistance-distance) / shadowMaxDistance);
    }

    return ((1-alpha) * (cAmbient * cBase +  cDiffuse * cBase + cSpecular) + caustic) * shadow;
}