-- Vertex

#include "vbInputData.vert"

void main() {

    computeData();
#if defined(SHADOW_PASS)
    gl_Position = VAR._vertexW;
#else
    gl_Position = dvd_ViewProjectionMatrix * VAR._vertexW;
    VAR._normalWV = dvd_NormalMatrixWV() * dvd_Normal;
#endif
}

-- Geometry.Shadow

layout(binding = BUFFER_LIGHT_SHADOW, std140) uniform dvd_ShadowBlock
{
    mat4 _lightVP[MAX_SPLITS_PER_LIGHT];
};

#if defined(USE_TRIANGLE_STRIP)
layout(triangle_strip, invocations = MAX_SPLITS_PER_LIGHT) in;
#else
layout(triangles, invocations = MAX_SPLITS_PER_LIGHT) in;
#endif

layout(triangle_strip, max_vertices = 3) out;

out vec4 geom_vertexWVP;

void main()
{
    if (gl_InvocationID < dvd_GSInvocationLimit) {
        mat4 vp = _lightVP[gl_InvocationID];
        for (int i = 0; i < gl_in.length(); ++i)
        {
            passVertex(i);
            geom_vertexWVP = vp * gl_in[i].gl_Position;
            gl_Layer = gl_InvocationID + dvd_shadowArrayOffset;
            gl_Position = geom_vertexWVP;
            EmitVertex();
        }
        EndPrimitive();
   }
}

-- Fragment
#if defined(USE_OPACITY_DIFFUSE) || defined(USE_OPACITY_MAP) || defined(USE_OPACITY_DIFFUSE_MAP)
#   define HAS_TRANSPARENCY
#endif

#if !defined(HAS_TRANSPARENCY)
layout(early_fragment_tests) in;
#endif

#if defined(SHADOW_PASS)
out vec2 _colourOut;
in vec4 geom_vertexWVP;
#endif

#include "nodeBufferedInput.cmn"
#include "materialData.frag"

vec2 computeMoments(in float depth) {
    // Compute partial derivatives of depth.  
    float dx = dFdx(depth);
    float dy = dFdy(depth);
    // Compute second moment over the pixel extents.  
    return vec2(depth, depth*depth + 0.25*(dx*dx + dy*dy));
}

void main() {
    if (getOpacity() < ALPHA_DISCARD_THRESHOLD) {
        discard;
    }

#if defined(SHADOW_PASS)
    // Adjusting moments (this is sort of bias per pixel) using partial derivative
    float depth = geom_vertexWVP.z / geom_vertexWVP.w;
    depth = depth * 0.5 + 0.5;
    //_colourOut = computeMoments(exp(DEPTH_EXP_WARP * depth));
    _colourOut = computeMoments(depth);
#endif

}

