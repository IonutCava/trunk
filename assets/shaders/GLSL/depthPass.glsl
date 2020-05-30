-- Fragment.PrePass

#include "prePass.frag"

void main() {
    NodeData data = dvd_Matrices[DATA_IDX];
    prepareData(data);

    writeOutput(data, TexCoords);
}

-- Fragment.Shadow

#if defined(USE_ALBEDO_ALPHA) || defined(USE_OPACITY_MAP)
#   define HAS_TRANSPARENCY
#endif

#if defined(HAS_TRANSPARENCY)
#include "materialData.frag"
#endif

void main() {
#if defined(HAS_TRANSPARENCY)
    NodeData data = dvd_Matrices[DATA_IDX];
    prepareData(data);

    mat4 colourMatrix = data._colourMatrix;
    if (getAlbedo(colourMatrix, TexCoords).a < INV_Z_TEST_SIGMA) {
        discard;
    }
#endif
}

--Fragment.Shadow.VSM

#if defined(USE_ALBEDO_ALPHA) || defined(USE_OPACITY_MAP)
#   define HAS_TRANSPARENCY
#endif

#include "vsm.frag"
out vec2 _colourOut;

#if defined(HAS_TRANSPARENCY)
#include "materialData.frag"
#endif


void main() {
#if defined(HAS_TRANSPARENCY)
    NodeData data = dvd_Matrices[DATA_IDX];
    prepareData(data);

    mat4 colourMatrix = data._colourMatrix;
    if (getAlbedo(colourMatrix, TexCoords).a < INV_Z_TEST_SIGMA) {
        discard;
    }
#endif

    _colourOut = computeMoments();
}