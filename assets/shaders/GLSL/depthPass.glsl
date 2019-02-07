-- Vertex

#include "vbInputData.vert"

#if defined(SHADOW_PASS)
out vec4 vert_vertexWVP;
#endif

void main() {

    computeData();

    gl_Position = dvd_ViewProjectionMatrix * VAR._vertexW;

#if defined(SHADOW_PASS)
    vert_vertexWVP = gl_Position;
#endif
}

-- Fragment.PrePass.AlphaDiscard

#include "materialData.frag"

void main() {
#if defined(HAS_TRANSPARENCY)
    if (getOpacity() < 1.0 - Z_TEST_SIGMA) {
        discard;
    }
#endif
}

-- Fragment.Shadow
#if defined(USE_OPACITY_DIFFUSE) || defined(USE_OPACITY_MAP) || defined(USE_OPACITY_DIFFUSE_MAP)
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
    if (getOpacity() < 1.0 - Z_TEST_SIGMA) {
        discard;
    }
#endif
    _colourOut = computeMoments();
}

