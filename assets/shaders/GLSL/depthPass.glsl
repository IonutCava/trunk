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
    if (getAlbedo(colourMatrix, TexCoords).a < 1.0f - Z_TEST_SIGMA) {
        discard;
    }
#endif
}

--Fragment.Shadow.VSM

#if defined(USE_ALBEDO_ALPHA) || defined(USE_OPACITY_MAP)
#   define HAS_TRANSPARENCY
#endif

#if !defined(USE_SEPARATE_VSM_PASS)
#include "vsm.frag"
out vec2 _colourOut;
#endif

#if defined(HAS_TRANSPARENCY)
#include "materialData.frag"
#endif


void main() {
#if defined(HAS_TRANSPARENCY)
    mat4 colourMatrix = dvd_Matrices[DATA_IDX]._colourMatrix;
    if (getAlbedo(colourMatrix, TexCoords).a < 1.0f - Z_TEST_SIGMA) {
        discard;
    }
#endif

#if !defined(USE_SEPARATE_VSM_PASS)
    _colourOut = computeMoments();
#endif
}