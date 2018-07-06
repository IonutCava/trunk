-- Vertex

#include "vbInputData.vert"
#include "lightingDefaults.vert"

uniform vec3 bbox_min;
uniform vec3 bbox_extent;

out vec4 _scrollingUV;
smooth out float distance;

smooth out float _waterDepth;

void main(void){

    computeData();

    VAR._texCoord = vec3((VAR._vertexW.xyz - bbox_min) / bbox_extent).sp;

    float minHeight = (worldMat * vec4(0.0, TERRAIN_MIN_HEIGHT, 0.0, 1.0)).y;
    _waterDepth = clamp(1.0 - (dvd_waterPositionsW.y - VAR[index]._vertexW.y) / (dvd_waterPositionsW.y - minHeight), 0.0, 1.0);

    computeLightVectors();

    float time2 = float(dvd_time) * 0.0001;
    vec2 noiseUV = VAR._texCoord * UNDERWATER_DIFFUSE_SCALE;
    _scrollingUV.st = noiseUV;
    _scrollingUV.pq = noiseUV + time2;
    _scrollingUV.s -= time2;

    distance = gl_ClipDistance[0];
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

in vec4 _scrollingUV;
smooth in float distance;
smooth in float _waterDepth;

layout(location = 0) out vec4 _colourOut;
layout(location = 1) out vec2 _normalOut;
layout(location = 2) out vec2 _velocityOut;

vec4 CausticsColour() {
    setProcessedNormal(VAR._normalWV);

    setAlbedo((texture(texWaterCaustics, _scrollingUV.st) +
               texture(texWaterCaustics, _scrollingUV.pq)) * 0.5);

    return getPixelColour(VAR._texCoord);
}

vec4 UnderwaterColour() {

    vec2 coords = VAR._texCoord * UNDERWATER_DIFFUSE_SCALE;
    setAlbedo(texture(texUnderwaterAlbedo, coords));

    vec3 tbn = normalize(2.0 * texture(texUnderwaterDetail, coords).rgb - 1.0);
    setProcessedNormal(getTBNMatrix() * tbn);

    return getPixelColour(VAR._texCoord);
}

vec4 UnderwaterMappingRoutine(){
    return mix(CausticsColour(), UnderwaterColour(), _waterDepth);
}

vec4 TerrainMappingRoutine(){ // -- HACK - Ionut
    setAlbedo(getTerrainAlbedo());

    if (dvd_lodLevel(VAR.dvd_instanceID) == 0 && dvd_renderDetailLevel > DETAIL_LOW) {
        setProcessedNormal(getTerrainNormal());
    } else {
        setProcessedNormal(VAR._normalWV);
    }

    return getPixelColour(VAR._texCoord);
}

void main(void) {
    bumpInit();

   _colourOut = mix(TerrainMappingRoutine(), UnderwaterMappingRoutine(), min(distance, 0.0));

   _normalOut = packNormal(getProcessedNormal());
   _velocityOut = velocityCalc(dvd_InvProjectionMatrix, getScreenPositionNormalised());
}
