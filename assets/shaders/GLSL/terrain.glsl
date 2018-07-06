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

    gl_Position = dvd_ViewProjectionMatrix * VAR._vertexW;
}

-- Fragment

#include "BRDF.frag"
#include "terrainSplatting.frag"

in vec2 _uv0;
in vec2 _uv1;

smooth in float _waterDepth;

layout(location = 0) out vec4 _colorOut;
layout(location = 1) out vec3 _normalOut;
//subroutine vec4 TerrainMappingType();

//subroutine(TerrainMappingType) 
vec4 computeLightInfoLOD1Frag() {
    vec4 color = vec4(0.0);
    getColorNormal(color);
    _normalOut = VAR._normalWV;
    return getPixelColor(VAR._texCoord, _normalOut);
}

//subroutine(TerrainMappingType)
vec4 computeLightInfoLOD0Frag() {
    vec4 color = vec4(0.0);
    getColorAndTBNNormal(color, _normalOut);
    return getPixelColor(VAR._texCoord, _normalOut);
}


vec4 CausticsColor() {
    return (texture(texWaterCaustics, _uv1) + texture(texWaterCaustics, _uv0)) * 0.5;
}

vec4 UnderwaterColor() {
    vec4 color = vec4(0.0);

    getColorAndTBNUnderwater(color, _normalOut);
    return getPixelColor(VAR._texCoord, _normalOut);
}

vec4 UnderwaterMappingRoutine(){
    return mix(CausticsColor(), UnderwaterColor(), _waterDepth);
}

//subroutine uniform TerrainMappingType TerrainMappingRoutine;

vec4 TerrainMappingRoutine(){ // -- HACK - Ionut
    return dvd_lodLevel == 0 ? computeLightInfoLOD0Frag() :  computeLightInfoLOD1Frag();
}

void main(void) {
   _colorOut = applyFog(gl_ClipDistance[0] > 0.0 ? TerrainMappingRoutine() : UnderwaterMappingRoutine());
}
