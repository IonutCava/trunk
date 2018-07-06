-- Vertex

#include "vbInputData.vert"

void main() {

    computeData();
    gl_Position = dvd_ViewProjectionMatrix * _vertexW;
}

-- Fragment.Shadow

in vec2 _texCoord;
in vec4 _vertexW;
out vec4 _colorOut;

#if defined(USE_OPACITY_DIFFUSE) || defined(USE_OPACITY_MAP) || defined(USE_OPACITY_DIFFUSE_MAP)
#define HAS_TRANSPARENCY
#if defined(USE_OPACITY_DIFFUSE)
uniform mat4 material;
#endif
#if defined(USE_OPACITY_MAP)
layout(binding = TEXTURE_OPACITY) uniform sampler2D texOpacityMap;
#endif
#if defined(USE_OPACITY_DIFFUSE_MAP)
layout(binding = TEXTURE_UNIT0)   uniform sampler2D texDiffuse0;
#endif
#endif

#if defined(COMPUTE_MOMENTS)
vec2 computeMoments(in float depth) {
    // Compute partial derivatives of depth.  
    float dx = dFdx(depth);
    float dy = dFdy(depth);
    // Compute second moment over the pixel extents.  
    return vec2(depth, depth*depth + 0.25*(dx*dx + dy*dy));
}
#endif

void main() {

    float alpha = 1.0;
#if defined(HAS_TRANSPARENCY)
#if defined(USE_OPACITY_DIFFUSE_MAP)
    alpha *= texture(texDiffuse0, _texCoord).a;
#endif
#if defined(USE_OPACITY_DIFFUSE)
    alpha *= dvd_MatDiffuse.a;
#endif
#if defined(USE_OPACITY_MAP)
    vec4 opacityMap = texture(texOpacityMap, _texCoord);
    alpha *= max(min(opacityMap.r, opacityMap.g), min(opacityMap.b, opacityMap.a));
#endif
    if (alpha < ALPHA_DISCARD_THRESHOLD) discard;
#endif


#if defined(COMPUTE_MOMENTS)
    // Adjusting moments (this is sort of bias per pixel) using partial derivative
    float linearz = gl_FragCoord.z;
    //_colorOut = vec4(computeMoments(exp(DEPTH_EXP_WARP * linearz)), 0.0, alpha);
    _colorOut = vec4(computeMoments(linearz), 0.0, alpha);
#else
    _colorOut = vec4(gl_FragCoord.w, 0.0, 0.0, alpha);
#endif
}

