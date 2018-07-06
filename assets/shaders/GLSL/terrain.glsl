-- Vertex

#include "vbInputData.vert"
#include "lightingDefaults.vert"

uniform vec3 bbox_min;
uniform vec3 bbox_extent;
uniform float dvd_waterHeight;
uniform float underwaterDiffuseScale;

out vec2 _uv0;
out vec2 _uv1;
smooth out float _waterDepth;

void main(void){

    computeData();

    VAR._texCoord = vec3((VAR._vertexW.xyz - bbox_min) / bbox_extent).sp;
    _waterDepth = 1.0 - (dvd_waterHeight - VAR._vertexW.y) / (dvd_waterHeight - bbox_min.y);
    computeLightVectors();

    float time2 = float(dvd_time) * 0.0001;
    vec2 noiseUV = VAR._texCoord * underwaterDiffuseScale;
    _uv0 = noiseUV;
    _uv1 = noiseUV + time2;
    _uv0.s -= time2;

    setClipPlanes(VAR._vertexW);

    gl_Position = dvd_ViewProjectionMatrix * VAR._vertexW;
}

-- Fragment

#define CUSTOM_MATERIAL_DATA

#include "BRDF.frag"
#include "terrainSplatting.frag"
#include "velocityCalc.frag"

float getOpacity() {
    return 1.0;
}

vec4 private_albedo = vec4(1.0);
void setAlbedo(in vec4 albedo) {
    private_albedo = albedo;
}

vec4 getAlbedo() {
    return private_albedo;
}

vec3 getEmissive() {
    return private_getEmissive();
}

vec3 getSpecular() {
    return private_getSpecular();
}

float getShininess() {
    return private_getShininess();
}

in vec2 _uv0;
in vec2 _uv1;

smooth in float _waterDepth;

layout(location = 0) out vec4 _colourOut;
layout(location = 1) out vec2 _normalOut;
layout(location = 2) out vec2 _velocityOut;

//subroutine vec4 TerrainMappingType();

//subroutine(TerrainMappingType) 
vec4 computeLightInfoLOD1Frag() {
    vec4 albedo = vec4(1.0);
    getColourNormal(albedo);
    setProcessedNormal(VAR._normalWV);

    return albedo;
}

//subroutine(TerrainMappingType)
vec4 computeLightInfoLOD0Frag() {
    vec4 albedo = vec4(1.0);
    vec3 normal = vec3(1.0);
    getColourAndTBNNormal(albedo, normal);
    setProcessedNormal(normal);

    return albedo;
}

vec4 CausticsColour() {
    return (texture(texWaterCaustics, _uv1) + 
            texture(texWaterCaustics, _uv0)) * 0.5;
}

vec4 UnderwaterColour() {
    vec4 albedo = vec4(1.0);
    vec3 normal = vec3(1.0);
    getColourAndTBNUnderwater(albedo, normal);
    setAlbedo(albedo);
    setProcessedNormal(normal);

    return getPixelColour(VAR._texCoord);
}

vec4 UnderwaterMappingRoutine(){
    return vec4(vec3(0.0), 1.0);//mix(CausticsColour(), UnderwaterColour(), _waterDepth);
}

//subroutine uniform TerrainMappingType TerrainMappingRoutine;

vec4 TerrainMappingRoutine(){ // -- HACK - Ionut
    setAlbedo(dvd_lodLevel == 0 ? computeLightInfoLOD0Frag()
                                : computeLightInfoLOD1Frag());

    return getPixelColour(VAR._texCoord);
}

void main(void) {
   _colourOut = ToSRGB(applyFog(gl_ClipDistance[0] > 0.0 ? TerrainMappingRoutine() : UnderwaterMappingRoutine()));
   _normalOut = packNormal(getProcessedNormal());
   _velocityOut = velocityCalc(dvd_InvProjectionMatrix, getScreenPositionNormalised());
}
