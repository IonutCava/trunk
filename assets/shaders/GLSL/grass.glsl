-- Vertex

#include "vbInputData.vert"
#include "nodeBufferedInput.cmn"
#include "vegetationData.cmn"
#include "utility.cmn"

layout(location = 0) flat out int _arrayLayerFrag;
layout(location = 1) out float _alphaFactor;

#define GRASS_DISPLACEMENT_DISTANCE 10.0f
#define GRASS_DISPLACEMENT_MAGNITUDE 2.0f

void computeFoliageMovementGrass(inout vec4 vertex, in float heightExtent) {
    float timeGrass = dvd_windDetails.w * MSToSeconds(dvd_time) * 0.25f;
    float cosX = cos(vertex.x);
    float sinX = sin(vertex.x);
    float halfScale = 0.5f * heightExtent;
    vertex.x += (halfScale * cos(timeGrass) * cosX * sinX) *dvd_windDetails.x;
    vertex.z += (halfScale * sin(timeGrass) * cosX * sinX) *dvd_windDetails.z;
}

vec3 rotate_vertex_position(vec3 position, vec4 q) {
    const vec3 v = position.xyz;
    return v + 2.0f * cross(q.xyz, cross(q.xyz, v) + q.w * v);
}

void main() {
    const NodeData nodeData = fetchInputData();

    VegetationData data = GrassData(gl_InstanceID);

#if defined(USE_CULL_DISTANCE)
    if (data.data.z > 2.5f) {
        gl_CullDistance[0] = -1.0f;
    }
    float scale = data.positionAndScale.w;
#else
    float scale = data.data.z > 2.5f ? 0.0f : data.positionAndScale.w;
#endif

#if defined(PRE_PASS)
    _alphaFactor = 1.0f;
#else
    _alphaFactor = saturate(data.data.z);
#endif
    _arrayLayerFrag = int(data.data.x);
    
    dvd_Vertex.xyz *= scale;
    dvd_Vertex.xyz = rotate_vertex_position(dvd_Vertex.xyz, data.orientationQuad);
    if (dvd_Vertex.y > 0.5f) {
        computeFoliageMovementGrass(dvd_Vertex, dvd_Vertex.y);
    }
    dvd_Vertex.y += 0.25f;
    VAR._vertexW = dvd_Vertex + vec4(data.positionAndScale.xyz, 0.0f);
    if (dvd_Vertex.y > 0.5f && data.data.w < GRASS_DISPLACEMENT_DISTANCE) {
        const vec3 toCamera = normalize(VAR._vertexW.xyz - dvd_cameraPosition.xyz);
        VAR._vertexW.xyz += vec3(GRASS_DISPLACEMENT_MAGNITUDE, 0.0f, GRASS_DISPLACEMENT_MAGNITUDE) * toCamera * ((GRASS_DISPLACEMENT_DISTANCE - data.data.w) / GRASS_DISPLACEMENT_DISTANCE);
    }

    mat3 normalMatrixWV = mat3(dvd_ViewMatrix) * mat3(nodeData._normalMatrixW);

    VAR._vertexWV = dvd_ViewMatrix * VAR._vertexW;
    VAR._vertexWVP = dvd_ProjectionMatrix * VAR._vertexWV;
    VAR._normalWV = normalize(normalMatrixWV * rotate_vertex_position(dvd_Normal, data.orientationQuad));

#if !defined(SHADOW_PASS)
    setClipPlanes();
#endif

    gl_Position = VAR._vertexWVP;
}

-- Fragment.Colour

layout(early_fragment_tests) in;

#define USE_SHADING_BLINN_PHONG

#include "BRDF.frag"

#include "utility.frag"
#include "output.frag"

layout(location = 0) flat in int _arrayLayerFrag;
layout(location = 1) in float _alphaFactor;

layout(binding = TEXTURE_UNIT0) uniform sampler2DArray texDiffuseGrass;

void main (void){
    NodeData data = dvd_Matrices[DATA_IDX];
    prepareData(data);

    const vec2 uv = VAR._texCoord;

    vec4 albedo = texture(texDiffuseGrass, vec3(uv, _arrayLayerFrag));
    albedo.a = min(albedo.a, _alphaFactor);

    writeOutput(getPixelColour(albedo, data, getNormalWV(uv), uv));
}

--Fragment.PrePass

// This is needed so that the inner "output" discards don't take place
// We still manually alpha-discard in main
#if defined(USE_ALPHA_DISCARD)
#undef USE_ALPHA_DISCARD
#endif

#include "prePass.frag"

layout(location = 0) flat in int _arrayLayerFrag;
layout(location = 1) in float _alphaFactor;

layout(binding = TEXTURE_UNIT0) uniform sampler2DArray texDiffuseGrass;

void main() {
    const float albedoAlpha = texture(texDiffuseGrass, vec3(VAR._texCoord, _arrayLayerFrag)).a;
    if (albedoAlpha * _alphaFactor < INV_Z_TEST_SIGMA) {
        discard;
    }

#if defined(HAS_PRE_PASS_DATA)
    NodeData data = dvd_Matrices[DATA_IDX];
    prepareData(data);

    writeOutput(data,
                VAR._texCoord,
                VAR._normalWV,
                getTBNViewDirection(),
                _alphaFactor);
#endif //HAS_PRE_PASS_DATA
}

--Fragment.Shadow.VSM

layout(location = 0) flat in int _arrayLayerFrag;
layout(location = 1) in float _alphaFactor;

layout(binding = TEXTURE_UNIT0) uniform sampler2DArray texDiffuseGrass;

#include "vsm.frag"
out vec2 _colourOut;

void main(void) {
    const float albedoAlpha = texture(texDiffuseGrass, vec3(VAR._texCoord, _arrayLayerFrag)).a;
    if (albedoAlpha * _alphaFactor < INV_Z_TEST_SIGMA) {
        discard;
    }

    _colourOut = computeMoments();
}