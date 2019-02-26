-- Vertex

#include "vbInputData.vert"

struct VegetationData {
    vec4 positionAndScale;
    vec4 orientationQuad;
    //x - width extent, y = height extent, z = array index, w - render
    vec4 data;
};

layout(std430, binding = BUFFER_GRASS_DATA) coherent readonly buffer dvd_transformBlock {
    VegetationData grassData[MAX_GRASS_INSTANCES];
};

flat out int _arrayLayerFrag;
flat out float _alphaFactor;

void computeFoliageMovementGrass(inout vec4 vertex, in float scaleFactor) {
    float timeGrass = dvd_windDetails.w * dvd_time * 0.00025; //to seconds
    float cosX = cos(vertex.x);
    float sinX = sin(vertex.x);
    float halfScale = 0.5*scaleFactor;
    vertex.x += (halfScale*cos(timeGrass) * cosX * sinX) *dvd_windDetails.x;
    vertex.z += (halfScale*sin(timeGrass) * cosX * sinX) *dvd_windDetails.z;
}

void main()
{
    computeDataNoClip();

    VegetationData data = grassData[VAR.dvd_instanceID];
    float LoD = data.data.w;
    float scale = data.positionAndScale.w;
    if (LoD > 2) {
        scale = 0.0f;
        //gl_CullDistance[0] = -0.01f;
    }

    if (dvd_Vertex.y > 0.75) {
        computeFoliageMovementGrass(dvd_Vertex, data.data.y);
    }

    _arrayLayerFrag = int(data.data.z);
    _alphaFactor = min(LoD, 1.0f);
    if (_alphaFactor > 0.5f) {
        _alphaFactor = 1.0f;
    }

    _alphaFactor = 1.0f;

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

flat in int _arrayLayerFrag;
flat in float _alphaFactor;

layout(binding = TEXTURE_UNIT0) uniform sampler2DArray texDiffuseGrass;

void main (void){
    vec4 albedo = texture(texDiffuseGrass, vec3(VAR._texCoord, _arrayLayerFrag));
    albedo.a *= _alphaFactor;

    vec3 normal = getNormal();

    mat4 colourMatrix = dvd_Matrices[VAR.dvd_baseInstance]._colourMatrix;
    writeOutput(getPixelColour(albedo, colourMatrix, normal), packNormal(normal));
}

--Fragment.PrePass

flat in int _arrayLayerFrag;
flat in float _alphaFactor;

layout(binding = TEXTURE_UNIT0) uniform sampler2DArray texDiffuseGrass;

void main() {
    vec4 colour = texture(texDiffuseGrass, vec3(VAR._texCoord, _arrayLayerFrag));
    if (colour.a * _alphaFactor < 1.0 - Z_TEST_SIGMA) {
        discard;
    }
}


--Fragment.Shadow

flat in int _arrayLayerFrag;
flat in float _alphaFactor;

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