-- Vertex

#include "vbInputData.vert"

void main() {

    computeData();

    gl_Position = dvd_ViewProjectionMatrix * VAR._vertexW;
}

-- Fragment.PrePass

layout(location = 1) out vec4 _normalAndVelocityOut;

#include "BRDF.frag"
#include "materialData.frag"
#include "velocityCalc.frag"

void main() {
    updateTexCoord();

    mat4 colourMatrix = dvd_Matrices[VAR.dvd_baseInstance]._colourMatrix;

#if defined(USE_ALPHA_DISCARD)
    if (getAlbedo(colourMatrix).a < 1.0f - Z_TEST_SIGMA) {
        discard;
    }
#endif

    _normalAndVelocityOut.rg = packNormal(normalize(getNormal()));
    _normalAndVelocityOut.ba = velocityCalc(dvd_InvProjectionMatrix, getScreenPositionNormalised());
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
    if (getAlbedo(colourMatrix).a < 1.0f - Z_TEST_SIGMA) {
        discard;
    }
#endif

    _colourOut = computeMoments();
}

