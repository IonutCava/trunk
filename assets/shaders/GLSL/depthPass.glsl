-- Fragment.PrePass

#include "prePass.frag"

void main() {
#if defined(HAS_PRE_PASS_DATA)
    NodeData data = dvd_Matrices[TRANSFORM_IDX];
    prepareData(data);

    writeOutput(data,
                VAR._texCoord,
                getNormalWV(VAR._texCoord));
#endif //HAS_PRE_PASS_DATA
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
    NodeData data = dvd_Matrices[TRANSFORM_IDX];
    prepareData(data);

    mat4 colourMatrix = data._colourMatrix;
    if (getAlbedo(colourMatrix, VAR._texCoord).a < INV_Z_TEST_SIGMA) {
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
    NodeData data = dvd_Matrices[TRANSFORM_IDX];
    prepareData(data);

    mat4 colourMatrix = data._colourMatrix;
    if (getAlbedo(colourMatrix, VAR._texCoord).a < INV_Z_TEST_SIGMA) {
        discard;
    }
#endif

    _colourOut = computeMoments();
}