-- Vertex

#include "vbInputData.vert"
#include "nodeBufferedInput.cmn"
#include "vegetationData.cmn"
#include "utility.cmn"

layout(location = 0) flat out int _arrayLayerFrag;
layout(location = 1) flat out float _alphaFactor;

void computeFoliageMovementGrass(inout vec4 vertex, in float heightExtent) {
    float timeGrass = dvd_windDetails.w * dvd_time * 0.00025f; //to seconds
    float cosX = cos(vertex.x);
    float sinX = sin(vertex.x);
    float halfScale = 0.25f * heightExtent;
    vertex.x += (halfScale * cos(timeGrass) * cosX * sinX) *dvd_windDetails.x;
    vertex.z += (halfScale * sin(timeGrass) * cosX * sinX) *dvd_windDetails.z;
}

void main()
{
    computeDataNoClip();

    VegetationData data = GrassData(VAR.dvd_instanceID);

    float LoD = data.data.z;
    float scale = data.positionAndScale.w;

#if defined(USE_CULL_DISTANCE)
    gl_CullDistance[0] = -0.01f * when_gt(LoD, 2.1f);
#else
    scale -= scale * when_gt(LoD, 2.1f);
#endif

    _arrayLayerFrag = int(data.data.x);
    _alphaFactor = min(1.0f - LoD, 1.0f);
    if (_alphaFactor > 0.5f) {
        _alphaFactor = 1.0f;
    }
    
    dvd_Vertex.xyz = rotate_vertex_position(dvd_Vertex.xyz * scale, data.orientationQuad);
    if (dvd_Vertex.y > 0.5f) {
        computeFoliageMovementGrass(dvd_Vertex, data.data.w * 0.5f);
    }

    VAR._vertexW = dvd_Vertex + vec4(data.positionAndScale.xyz, 0.0f);

    VAR._vertexWV = dvd_ViewMatrix * VAR._vertexW;
    VAR._normalWV = normalize(mat3(dvd_ViewMatrix) * /*rotate_vertex_position(dvd_Normal, data.orientationQuad)*/ -dvd_Normal);

#if !defined(SHADOW_PASS)
    setClipPlanes(VAR._vertexW);
#endif

    gl_Position = dvd_ProjectionMatrix * VAR._vertexWV;
}

-- Fragment.Colour

layout(early_fragment_tests) in;

#define USE_SHADING_BLINN_PHONG
#define NO_SPECULAR
#define NEED_DEPTH_TEXTURE

#include "BRDF.frag"

#include "utility.frag"
#include "output.frag"

layout(location = 0) flat in int _arrayLayerFrag;
layout(location = 1) flat in float _alphaFactor;

layout(binding = TEXTURE_UNIT0) uniform sampler2DArray texDiffuseGrass;

void main (void){
    const vec2 uv = TexCoords;

    vec4 albedo = texture(texDiffuseGrass, vec3(uv, _arrayLayerFrag));
    albedo.a *= _alphaFactor;

    mat4 colourMatrix = dvd_Matrices[DATA_IDX]._colourMatrix;
    writeOutput(getPixelColour(albedo, colourMatrix, getNormal(uv), uv));
}

--Fragment.PrePass

#include "prePass.frag"

layout(location = 0) flat in int _arrayLayerFrag;
layout(location = 1) flat in float _alphaFactor;

layout(binding = TEXTURE_UNIT0) uniform sampler2DArray texDiffuseGrass;

// This is needed so that the inner "outputNoVelocity" discards don't take place
// We still manually alpha-discard in main
#if defined(USE_ALPHA_DISCARD)
#undef USE_ALPHA_DISCARD
#endif

void main() {
    vec4 colour = texture(texDiffuseGrass, vec3(VAR._texCoord, _arrayLayerFrag));
    float alpha = colour.a * _alphaFactor;
    if (alpha <= 1.0f - Z_TEST_SIGMA) {
        discard;
    }

    outputNoVelocity(VAR._texCoord, _alphaFactor);
}


--Fragment.Shadow

layout(location = 0) flat in int _arrayLayerFrag;
layout(location = 1) flat in float _alphaFactor;

#include "vsm.frag"

layout(binding = TEXTURE_UNIT0) uniform sampler2DArray texDiffuseGrass;

out vec2 _colourOut;

void main(void) {
    vec4 colour = texture(texDiffuseGrass, vec3(VAR._texCoord, _arrayLayerFrag));
    float alpha = colour.a * _alphaFactor;
    if (alpha <= 1.0f - Z_TEST_SIGMA) {
        discard;
    }

    _colourOut = computeMoments();
}