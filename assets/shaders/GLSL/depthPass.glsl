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

void main()
{
    if (gl_InvocationID < dvd_GSInvocationLimit) {
        mat4 vp = _lightVP[gl_InvocationID];
        for (int i = 0; i < gl_in.length(); ++i)
        {
            passVertex(i);
            gl_Layer = gl_InvocationID + dvd_shadowArrayOffset;
            gl_Position = vp * gl_in[i].gl_Position;
            EmitVertex();
        }
        EndPrimitive();
   }
}

-- Fragment
#if !defined(HAS_TRANSPARENCY)
layout(early_fragment_tests) in;
#endif

#if defined(SHADOW_PASS)
out vec2 _colourOut;
#endif

#include "nodeBufferedInput.cmn"

#if defined(USE_OPACITY_DIFFUSE) || defined(USE_OPACITY_MAP)
#define HAS_TRANSPARENCY
#if defined(USE_OPACITY_DIFFUSE)
uniform mat4 material;
#endif
#if defined(USE_OPACITY_MAP)
layout(binding = TEXTURE_OPACITY) uniform sampler2D texOpacityMap;
#endif
#if defined(USE_OPACITY_DIFFUSE) && !defined(SKIP_TEXTURES)
layout(binding = TEXTURE_UNIT0)   uniform sampler2D texDiffuse0;
layout(binding = TEXTURE_UNIT1)   uniform sampler2D texDiffuse1;
#endif
#endif

vec2 computeMoments(in float depth) {
    // Compute partial derivatives of depth.  
    float dx = dFdx(depth);
    float dy = dFdy(depth);
    // Compute second moment over the pixel extents.  
    return vec2(depth, depth*depth + 0.25*(dx*dx + dy*dy));
}

void main() {
    float alpha = 1.0;
#if defined(HAS_TRANSPARENCY)
#if defined(USE_OPACITY_DIFFUSE)
#   if defined(SKIP_TEXTURES)
        alpha *= dvd_MatDiffuse.a;
#   else 
    if (dvd_BufferIntegerValues().w == 0) {
        alpha *= texture(texDiffuse0, VAR._texCoord).a;
    } else {
        alpha *= texture(texDiffuse1, VAR._texCoord).a;
    }
#   endif
#endif

#if defined(USE_OPACITY_MAP)
    vec4 opacityMap = texture(texOpacityMap, VAR._texCoord);
    alpha *= max(min(opacityMap.r, opacityMap.g), min(opacityMap.b, opacityMap.a));
#endif
    if (alpha < ALPHA_DISCARD_THRESHOLD) discard;
#endif

#if defined(SHADOW_PASS)
    // Adjusting moments (this is sort of bias per pixel) using partial derivative
    //_colourOut = vec4(computeMoments(exp(DEPTH_EXP_WARP * gl_FragCoord.z)), 0.0, alpha);
    _colourOut = computeMoments(gl_FragCoord.z);
#endif

}

