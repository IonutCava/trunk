-- Fragment.PrePass

#include "prePass.frag"

void main() {
    outputWithVelocity(TexCoords, 1.0f, computeDepth(VAR._vertexWV));
}

-- Fragment.Shadow

#if defined(USE_ALBEDO_ALPHA) || defined(USE_OPACITY_MAP)
#   define HAS_TRANSPARENCY
#endif

out vec2 _colourOut;

#if defined(HAS_TRANSPARENCY)
#include "materialData.frag"
#endif

#include "vsm.frag"

void main() {
#if defined(HAS_TRANSPARENCY)
    mat4 colourMatrix = dvd_Matrices[DATA_IDX]._colourMatrix;
    if (getAlbedo(colourMatrix, TexCoords).a < 1.0f - Z_TEST_SIGMA) {
        discard;
    }
#endif

    _colourOut = computeMoments();
}

