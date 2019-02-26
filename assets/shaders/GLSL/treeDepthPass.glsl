-- Vertex

#include "vbInputData.vert"

struct VegetationData {
    vec4 positionAndScale;
    vec4 orientationQuad;
    //x - width extent, y = height extent, z = array index, w - render
    vec4 data;
};

layout(std430, binding = BUFFER_TREE_DATA) coherent readonly buffer dvd_transformBlock {
    VegetationData grassData[MAX_TREE_INSTANCES];
};

void computeFoliageMovementGrass(inout vec4 vertex, in float scaleFactor) {
    float timeGrass = dvd_windDetails.w * dvd_time * 0.00025; //to seconds
    float cosX = cos(vertex.x);
    float sinX = sin(vertex.x);
    float halfScale = 0.5 * scaleFactor;
    vertex.x += (halfScale * cos(timeGrass) * cosX * sinX) * dvd_windDetails.x;
    vertex.z += (halfScale * sin(timeGrass) * cosX * sinX) * dvd_windDetails.z;
}

void main(void) {

    computeDataNoClip();

    VegetationData data = grassData[VAR.dvd_instanceID];
    float scale = data.positionAndScale.w + 10.0f;

    if (dvd_Vertex.y > 0.75) {
        computeFoliageMovementGrass(dvd_Vertex, data.data.y);
    }

    dvd_Vertex.xyz = rotate_vertex_position(dvd_Vertex.xyz * scale, data.orientationQuad);
    VAR._vertexW = dvd_Vertex + vec4(data.positionAndScale.xyz, 0.0f);

    VAR._vertexWV = dvd_ViewMatrix * VAR._vertexW;
#if !defined(DEPTH_PASS)
    VAR._normalWV = normalize(mat3(dvd_ViewMatrix) * /*rotate_vertex_position(dvd_Normal, data.orientationQuad)*/ -dvd_Normal);
#endif

#if !defined(SHADOW_PASS)
    setClipPlanes(VAR._vertexW);
#endif

    //Compute the final vert position
    gl_Position = dvd_ProjectionMatrix * VAR._vertexWV;

}

-- Fragment.PrePass.AlphaDiscard
#include "materialData.frag"

void main() {
    mat4 colourMatrix = dvd_Matrices[VAR.dvd_baseInstance]._colourMatrix;

    if (getAlbedo(colourMatrix).a < 1.0 - Z_TEST_SIGMA) {
        discard;
    }
}

-- Fragment.Shadow
#if defined(USE_ALBEDO_ALPHA) || defined(USE_OPACITY_MAP)
#   define HAS_TRANSPARENCY
#endif

out vec2 _colourOut;

#if defined(HAS_TRANSPARENCY)
#include "materialData.frag"
#else
#include "nodeBufferedInput.cmn"
#endif
#include "utility.frag"

#include "vsm.frag"

void main() {
#if defined(HAS_TRANSPARENCY)
    mat4 colourMatrix = dvd_Matrices[VAR.dvd_baseInstance]._colourMatrix;
    if (getAlbedo(colourMatrix).a < 1.0 - Z_TEST_SIGMA) {
        discard;
    }
#endif

    _colourOut = computeMoments();
}

