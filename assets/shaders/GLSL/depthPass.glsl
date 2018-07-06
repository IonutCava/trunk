-- Vertex.Shadow

#if defined(USE_GPU_SKINNING)
#include "boneTransforms.vert"
#endif


uniform int  dvd_drawID = 0;
uniform mat4 dvd_WorldMatrix[MAX_INSTANCES];

out vec2 _texCoord;

void main(void){
    vec4 dvd_Vertex = vec4(inVertexData, 1.0);


#if defined(USE_GPU_SKINNING)
    vec3 dvd_Normal = vec3(1.0);
    applyBoneTransforms(dvd_Vertex, dvd_Normal, 2);
#endif

    _texCoord = inTexCoordData;
    vec4 wVertex = dvd_WorldMatrix[dvd_drawID] * dvd_Vertex;
    setClipPlanes(wVertex);
    gl_Position = dvd_ViewProjectionMatrix * wVertex;
}

-- Fragment.Shadow

in vec2 _texCoord;
out vec2 _colorOut;

#if defined(USE_OPACITY_DIFFUSE) || defined(USE_OPACITY) || defined(USE_OPACITY_MAP) || defined(USE_OPACITY_DIFFUSE_MAP)
#define HAS_TRANSPARENCY
#if defined(USE_OPACITY)
uniform float opacity = 1.0;
#endif
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

vec2 computeMoments(in float depth) {
    // Compute partial derivatives of depth.  
    float dx = dFdx(depth);
    float dy = dFdy(depth);
    // Compute second moment over the pixel extents.  
    return vec2(depth, depth*depth + 0.25*(dx*dx + dy*dy));
}

void main(){

#if defined(HAS_TRANSPARENCY)
    float alpha = 1.0;
#if defined(USE_OPACITY_DIFFUSE_MAP)
    alpha *= texture(texDiffuse0, _texCoord).a;
#endif
#if defined(USE_OPACITY_DIFFUSE)
    alpha *= material[1].w;
#endif
#if defined(USE_OPACITY)
    alpha *= opacity;
#endif
#if defined(USE_OPACITY_MAP)
    vec4 opacityMap = texture(texOpacityMap, _texCoord);
    alpha *= max(min(opacityMap.r, opacityMap.g), min(opacityMap.b, opacityMap.a));
#endif
    if (alpha < ALPHA_DISCARD_THRESHOLD) discard;
#endif


    // Adjusting moments (this is sort of bias per pixel) using partial derivative
    float linearz = gl_FragCoord.z;
    //_colorOut = computeMoments(exp(DEPTH_EXP_WARP * linearz));
    _colorOut = computeMoments( linearz);
}

-- Vertex.PrePass

#include "vbInputData.vert"

void main() {

    computeData();
    gl_Position = dvd_ViewProjectionMatrix * _vertexW;
}

-- Fragment.PrePass

in vec2 _texCoord;
in vec4 _vertexW;
out vec4 _colorOut;

#if defined(USE_OPACITY_DIFFUSE) || defined(USE_OPACITY) || defined(USE_OPACITY_MAP) || defined(USE_OPACITY_DIFFUSE_MAP)
#define HAS_TRANSPARENCY
#if defined(USE_OPACITY)
uniform float opacity = 1.0;
#endif
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

void main(){
    float alpha = 1.0;
#if defined(HAS_TRANSPARENCY)
#if defined(USE_OPACITY_DIFFUSE_MAP)
    alpha *= texture(texDiffuse0, _texCoord).a;
#endif
#if defined(USE_OPACITY_DIFFUSE)
    alpha *= material[1].w;
#endif
#if defined(USE_OPACITY)
    alpha *= opacity;
#endif
#if defined(USE_OPACITY_MAP)
    vec4 opacityMap = texture(texOpacityMap, _texCoord);
    alpha *= max(min(opacityMap.r, opacityMap.g), min(opacityMap.b, opacityMap.a));
#endif
    if (alpha < ALPHA_DISCARD_THRESHOLD) discard;
#endif

    _colorOut = vec4(gl_FragCoord.w, 0.0, 0.0, alpha);
}