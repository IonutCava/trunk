-- Vertex

#include "vbInputData.vert"
#include "nodeBufferedInput.cmn"
#include "vegetationData.cmn"

layout(location = 0) flat out int _arrayLayerFrag;
layout(location = 1) flat out float _alphaFactor;

void computeFoliageMovementGrass(inout vec4 vertex, in float heightExtent) {
    float timeGrass = dvd_windDetails.w * dvd_time * 0.00025f; //to seconds
    float cosX = cos(vertex.x);
    float sinX = sin(vertex.x);
    float halfScale = 0.5f * heightExtent;
    vertex.x += (halfScale * cos(timeGrass) * cosX * sinX) *dvd_windDetails.x;
    vertex.z += (halfScale * sin(timeGrass) * cosX * sinX) *dvd_windDetails.z;
}

void main()
{
    computeDataNoClip();

    VegetationData data = GrassData(VAR.dvd_instanceID);

    float LoD = data.data.z;
    float scale = data.positionAndScale.w;

    if (LoD > 2.0f) {
#if defined(USE_CULL_DISTANCE)
        gl_CullDistance[0] = -0.01f;
#else
        scale = 0.0f;
#endif
    }

    if (dvd_Vertex.y > 0.75) {
        //computeFoliageMovementGrass(dvd_Vertex, data.data.w);
    }

    _arrayLayerFrag = int(data.data.x);
    _alphaFactor = min(1.0f - LoD, 1.0f);
    if (_alphaFactor > 0.5f) {
        _alphaFactor = 1.0f;
    }
    
    dvd_Vertex.xyz = rotate_vertex_position(dvd_Vertex.xyz * scale, data.orientationQuad);
    VAR._vertexW = dvd_Vertex + vec4(data.positionAndScale.xyz, 0.0f);

    VAR._vertexWV = dvd_ViewMatrix * VAR._vertexW;
    VAR._normalWV = normalize(mat3(dvd_ViewMatrix) * /*rotate_vertex_position(dvd_Normal, data.orientationQuad)*/ -dvd_Normal);

    //computeLightVectors();
#if !defined(SHADOW_PASS)
    setClipPlanes(VAR._vertexW);
#endif

    gl_Position = dvd_ProjectionMatrix * VAR._vertexWV;
}

-- Fragment.Colour

layout(early_fragment_tests) in;

#define USE_SHADING_BLINN_PHONG
#define NO_SPECULAR

#include "BRDF.frag"

#include "utility.frag"
#include "output.frag"

layout(location = 0) flat in int _arrayLayerFrag;
layout(location = 1) flat in float _alphaFactor;

layout(binding = TEXTURE_UNIT0) uniform sampler2DArray texDiffuseGrass;

void main (void){
    vec2 uv = getTexCoord();

    vec4 albedo = texture(texDiffuseGrass, vec3(uv, _arrayLayerFrag));
    albedo.a *= _alphaFactor;

    vec3 normal = getNormal(uv);

    mat4 colourMatrix = dvd_Matrices[VAR.dvd_baseInstance]._colourMatrix;
    writeOutput(getPixelColour(albedo, colourMatrix, normal, uv));
}

--Fragment.PrePass

#include "prePass.frag"

layout(location = 0) flat in int _arrayLayerFrag;
layout(location = 1) flat in float _alphaFactor;

layout(binding = TEXTURE_UNIT0) uniform sampler2DArray texDiffuseGrass;

#if defined(USE_ALPHA_DISCARD)
#undef USE_ALPHA_DISCARD
#endif

void main() {
    vec4 colour = texture(texDiffuseGrass, vec3(VAR._texCoord, _arrayLayerFrag));
    if (colour.a * _alphaFactor < 1.0 - Z_TEST_SIGMA) {
        discard;
    }

    outputNoVelocity(_alphaFactor);
}


--Fragment.Shadow

layout(location = 0) flat in int _arrayLayerFrag;
layout(location = 1) flat in float _alphaFactor;

#include "vsm.frag"

layout(binding = TEXTURE_UNIT0) uniform sampler2DArray texDiffuseGrass;

out vec2 _colourOut;

void main(void) {
    vec4 colour = texture(texDiffuseGrass, vec3(VAR._texCoord, _arrayLayerFrag));
    if (colour.a * _alphaFactor < 1.0 - Z_TEST_SIGMA) {
        discard;
    }

    _colourOut = computeMoments();
}