-- Vertex

#include "vbInputData.vert"
#include "nodeBufferedInput.cmn"
#include "vegetationData.cmn"
#include "utility.cmn"

layout(location = 0) flat out int _arrayLayerFrag;
layout(location = 1) out float _alphaFactor;

void computeFoliageMovementGrass(inout vec4 vertex, in float heightExtent) {
    float timeGrass = dvd_windDetails.w * dvd_time * 0.00025f; //to seconds
    float cosX = cos(vertex.x);
    float sinX = sin(vertex.x);
    float halfScale = 0.25f * heightExtent;
    vertex.x += (halfScale * cos(timeGrass) * cosX * sinX) *dvd_windDetails.x;
    vertex.z += (halfScale * sin(timeGrass) * cosX * sinX) *dvd_windDetails.z;
}

void main() {
    computeDataNoClip();

    VegetationData data = GrassData(VAR.dvd_instanceID);

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
    
    dvd_Vertex.xyz = rotate_vertex_position(dvd_Vertex.xyz * scale, data.orientationQuad);
    if (dvd_Vertex.y > 0.5f) {
        computeFoliageMovementGrass(dvd_Vertex, data.data.w * 0.5f);
    }

    VAR._vertexW = dvd_Vertex + vec4(data.positionAndScale.xyz, 0.0f);

    VAR._vertexWV = dvd_ViewMatrix * VAR._vertexW;
    VAR._vertexWVP = dvd_ProjectionMatrix * VAR._vertexWV;
    VAR._normalWV = normalize(mat3(dvd_ViewMatrix) * dvd_Normal);// rotate_vertex_position(-dvd_Normal, data.orientationQuad));

#if !defined(SHADOW_PASS)
    setClipPlanes(VAR._vertexW);
#endif

    gl_Position = VAR._vertexWVP;
}

-- Fragment.Colour

//layout(early_fragment_tests) in;

#define USE_DEFERRED_NORMALS
#define USE_SHADING_BLINN_PHONG

#include "BRDF.frag"

#include "utility.frag"
#include "output.frag"

layout(location = 0) flat in int _arrayLayerFrag;
layout(location = 1) in float _alphaFactor;

layout(binding = TEXTURE_UNIT0) uniform sampler2DArray texDiffuseGrass;

void main (void){
    const vec2 uv = TexCoords;

    vec4 albedo = texture(texDiffuseGrass, vec3(uv, _arrayLayerFrag));
    albedo.a = min(albedo.a, _alphaFactor);

    mat4 colourMatrix = dvd_Matrices[DATA_IDX]._colourMatrix;
    writeOutput(getPixelColour(albedo, colourMatrix, getNormal(uv), uv));
    //writeOutput(albedo);
}

--Fragment.PrePass

#define USE_DEFERRED_NORMALS
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
    if (albedoAlpha * _alphaFactor < 1.0f - Z_TEST_SIGMA) {
        discard;
    }

    writeOutput(VAR._texCoord, _alphaFactor);
}

--Fragment.Shadow.VSM

layout(location = 0) flat in int _arrayLayerFrag;
layout(location = 1) in float _alphaFactor;

layout(binding = TEXTURE_UNIT0) uniform sampler2DArray texDiffuseGrass;

#include "vsm.frag"
out vec2 _colourOut;

void main(void) {
    const float albedoAlpha = texture(texDiffuseGrass, vec3(VAR._texCoord, _arrayLayerFrag)).a;
    if (albedoAlpha * _alphaFactor < 1.0f - Z_TEST_SIGMA) {
        discard;
    }

    _colourOut = computeMoments();
}