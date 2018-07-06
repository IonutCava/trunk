-- Vertex

#include "vbInputData.vert"

void main() {

    computeData();
#if defined(COMPUTE_MOMENTS)
    gl_Position = _vertexW;
#else
    gl_Position = dvd_ViewProjectionMatrix * _vertexW;
#endif
}

-- Geometry.Shadow

layout(binding = BUFFER_LIGHT_SHADOW, std140) uniform dvd_ShadowBlock
{
    mat4  _lightVP[MAX_SPLITS_PER_LIGHT];
    vec4  _floatValues;
    vec4  _lightPosition[MAX_SPLITS_PER_LIGHT];
};

in vec2 _texCoord[3];
out vec2 _texCoordOut;


#if defined(USE_TRIANGLE_STRIP)
layout(triangle_strip, invocations = MAX_SPLITS_PER_LIGHT) in;
#else
layout(triangles, invocations = MAX_SPLITS_PER_LIGHT) in;
#endif

layout(triangle_strip, max_vertices = 3) out;

out int gl_Layer;

void main()
{
    for (int i = 0; i < 3; i++)
    {
        gl_Layer = gl_InvocationID;
        _texCoordOut = _texCoord[i];
        gl_Position = /*_lightVP[gl_InvocationID] * */gl_in[i].gl_Position;
        EmitVertex();
    }
    EndPrimitive();
}

-- Fragment

#if defined(COMPUTE_MOMENTS)
in vec2 _texCoordOut;
#else
in vec2 _texCoord;
#endif

out vec4 _colorOut;

#if defined(COMPUTE_MOMENTS)
in int gl_Layer;
#endif

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
#if defined(COMPUTE_MOMENTS)
    vec2 _texCoord = _texCoordOut;
#endif

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
    vec2 _texCoord = _texCoordOut;
    // Adjusting moments (this is sort of bias per pixel) using partial derivative
    float linearz = gl_FragCoord.z;
    //_colorOut = vec4(computeMoments(exp(DEPTH_EXP_WARP * linearz)), 0.0, alpha);
    _colorOut = vec4(computeMoments(linearz), 0.0, alpha);
    if (gl_Layer == 1) _colorOut = vec4(1.0, 0.0, 0.0, 1.0);
    if (gl_Layer == 2) _colorOut = vec4(0.0, 1.0, 0.0, 1.0);;
    if (gl_Layer == 3) _colorOut = vec4(0.0, 0.0, 1.0, 1.0);;
    if (gl_Layer == 4) _colorOut = vec4(1.0, 1.0, 0.0, 1.0);;

#else
    _colorOut = vec4(gl_FragCoord.w, 0.0, 0.0, alpha);
#endif
}

