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
in vec4 vert_vertexWVP;

#if defined(HAS_TRANSPARENCY)
#include "materialData.frag"
#else
#include "nodeBufferedInput.cmn"
#endif
#include "utility.frag"

vec2 computeMoments(in float depth) {
    // Compute partial derivatives of depth.
    float dx = dFdx(depth);
    float dy = dFdy(depth);
    // Compute second moment over the pixel extents.
    return vec2(depth, pow(depth, 2.0) + 0.25 * (dx*dx + dy*dy));
}

void main() {
#if defined(HAS_TRANSPARENCY)
    if (getOpacity() < 1.0 - Z_TEST_SIGMA) {
        discard;
    }
#endif

    // Adjusting moments (this is sort of bias per pixel) using partial derivative
    float depth = vert_vertexWVP.z / vert_vertexWVP.w;
    //depth = depth * 0.5 + 0.5;
    //_colourOut = computeMoments(exp(DEPTH_EXP_WARP * depth));
    _colourOut = computeMoments(depth);
}

