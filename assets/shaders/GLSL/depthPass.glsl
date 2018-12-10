-- Vertex

#include "vbInputData.vert"

out vec4 vert_vertexWVP;

void main() {

    computeData();

    gl_Position = dvd_ViewProjectionMatrix * VAR._vertexW;
    vert_vertexWVP = gl_Position;
    VAR._normalWV = dvd_NormalMatrixWV(VAR.dvd_instanceID) * dvd_Normal;
}


-- Fragment
#if defined(USE_OPACITY_DIFFUSE) || defined(USE_OPACITY_MAP) || defined(USE_OPACITY_DIFFUSE_MAP)
#   define HAS_TRANSPARENCY
#endif

#if defined(SHADOW_PASS) && !defined(HAS_TRANSPARENCY)
layout(early_fragment_tests) in;
#endif

#if defined(SHADOW_PASS)
out vec2 _colourOut;
in vec4 vert_vertexWVP;
#endif

#if defined(HAS_TRANSPARENCY)
#include "materialData.frag"
#else
#include "nodeBufferedInput.cmn"
#endif
#include "utility.frag"

#if defined(SHADOW_PASS)
vec2 computeMoments(in float depth) {
    // Compute partial derivatives of depth.
    float dx = dFdx(depth);
    float dy = dFdy(depth);
    // Compute second moment over the pixel extents.
    return vec2(depth, (depth * depth) + 0.25 * (dx*dx + dy*dy));
}
#endif

void main() {
#if defined(HAS_TRANSPARENCY)
    if (getOpacity() < 1.0 - Z_TEST_SIGMA) {
        discard;
    }
#endif

#if defined(SHADOW_PASS)
    // Adjusting moments (this is sort of bias per pixel) using partial derivative
    float depth = vert_vertexWVP.z;// gl_FragCoord.z;//vert_vertexWVP.z / vert_vertexWVP.w;
    //depth = depth * 0.5 + 0.5;
    //_colourOut = computeMoments(exp(DEPTH_EXP_WARP * depth));
    _colourOut = computeMoments(depth);
#endif

}

