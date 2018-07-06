-- Vertex

#include "vbInputData.vert"
#include "lightingDefaults.vert"

uniform vec3 bbox_min;
uniform vec3 bbox_extent;
uniform float dvd_waterHeight;
uniform float dvd_time;
uniform float underwaterDiffuseScale;

out vec2 _uv0;
out vec2 _uv1;
smooth out float _waterDepth;

void main(void){

    computeData();

    _texCoord = vec3((_vertexW.xyz - bbox_min) / bbox_extent).sp;
    _waterDepth = 1.0 - (dvd_waterHeight - _vertexW.y) / (dvd_waterHeight - bbox_min.y);
    computeLightVectors();

    float time2 = dvd_time * 0.0001;
    vec2 noiseUV = _texCoord * underwaterDiffuseScale;
    _uv0 = noiseUV;
    _uv1 = noiseUV + time2;
    _uv0.s -= time2;

    gl_Position = dvd_ViewProjectionMatrix * _vertexW;
}

-- Fragment

#include "phong_lighting.frag"
#include "terrainSplatting.frag"

in vec2 _uv0;
in vec2 _uv1;
smooth in float _waterDepth;

out vec4 _colorOut;

//subroutine vec4 TerrainMappingType();

//subroutine(TerrainMappingType) 
vec4 computeLightInfoLOD1Frag() {
    vec4 color = vec4(0.0);
    getColorNormal(color);
    return Phong(_texCoord, _normalWV, color);
}

//subroutine(TerrainMappingType)
vec4 computeLightInfoLOD0Frag() {
    vec4 color = vec4(0.0);
    vec3 tbn = vec3(0.0);
    getColorAndTBNNormal(color, tbn);
    return Phong(_texCoord, tbn, color);
}


vec4 CausticsColor() {
    return (texture(texWaterCaustics, _uv1) + texture(texWaterCaustics, _uv0)) * 0.5;
}

vec4 UnderwaterColor() {
    vec4 color = vec4(0.0);
    vec3 tbn = vec3(0.0);
    getColorAndTBNUnderwater(color, tbn);

    return Phong(_texCoord, tbn, color);
}

vec4 UnderwaterMappingRoutine(){
    return mix(CausticsColor(), UnderwaterColor(), _waterDepth);
}

//subroutine uniform TerrainMappingType TerrainMappingRoutine;

vec4 TerrainMappingRoutine(){ // -- HACK - Ionut
    return lodLevel == 0 ? computeLightInfoLOD0Frag() :  computeLightInfoLOD1Frag();
}

void main(void) {
   _colorOut = applyFog(gl_ClipDistance[0] > 0.0 ? TerrainMappingRoutine() : UnderwaterMappingRoutine());
}
