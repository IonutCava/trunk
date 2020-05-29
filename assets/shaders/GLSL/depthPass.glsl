-- Fragment.PrePass

#include "prePass.frag"

void main() {
    writeOutput(TexCoords);
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
    mat4 colourMatrix = dvd_Matrices[DATA_IDX]._colourMatrix;
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
    mat4 colourMatrix = dvd_Matrices[DATA_IDX]._colourMatrix;
    if (getAlbedo(colourMatrix, TexCoords).a < INV_Z_TEST_SIGMA) {
        discard;
    }
#endif

    _colourOut = computeMoments();
}