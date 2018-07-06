-- Vertex.Shadow

#include "clippingPlanes.vert"
#if defined(USE_GPU_SKINNING)
#include "boneTransforms.vert"
#endif

layout(std140) uniform dvd_MatrixBlock
{
    mat4 dvd_ProjectionMatrix;
    mat4 dvd_ViewMatrix;
    mat4 dvd_ViewProjectionMatrix;
};

uniform mat4 dvd_WorldMatrix;//[MAX_INSTANCES];

out vec2 _texCoord;

void main(void){
    vec4 dvd_Vertex = vec4(inVertexData, 1.0);


#if defined(USE_GPU_SKINNING)
    vec3 dvd_Normal = vec3(1.0);
    applyBoneTransforms(dvd_Vertex, dvd_Normal, 2);
#endif

    _texCoord = inTexCoordData;
    vec4 wVertex = dvd_WorldMatrix * dvd_Vertex;
    setClipPlanes(wVertex);
    gl_Position = dvd_ViewProjectionMatrix * wVertex;
}

-- Fragment.Shadow

#if defined(USE_OPACITY_MAP) || defined(USE_OPACITY_DIFFUSE_MAP)
in vec2 _texCoord;
#endif

#if defined(USE_OPACITY)
uniform float opacity = 1.0;
#elif defined(USE_OPACITY_MAP)
uniform sampler2D texOpacityMap;
#elif defined(USE_OPACITY_DIFFUSE_MAP)
uniform sampler2D texDiffuse0;
#elif defined(USE_OPACITY_DIFFUSE)
uniform mat4      material;
#endif

uniform vec2 dvd_zPlanes;

out vec2 _colorOut;

vec2 computeMoments(in float depth) {
    vec2 moments;

    moments.x = depth;

    // Compute partial derivatives of depth.  
    float dx = dFdx(depth);
    float dy = dFdy(depth);

    // Compute second moment over the pixel extents.  
    //moments.y = Depth*Depth + 0.25*(dx*dx + dy*dy);
    moments.y = depth * depth;
    return moments;
}

void main(){

#if defined(USE_OPACITY_DIFFUSE)
    if (material[1].w < ALPHA_DISCARD_THRESHOLD) discard;
#elif defined(USE_OPACITY)
    if (opacity < ALPHA_DISCARD_THRESHOLD) discard;
#elif defined(USE_OPACITY_MAP)
    vec4 opacityMap = texture(texOpacityMap, _texCoord);
    if (max(min(opacityMap.r, opacityMap.g), min(opacityMap.b, opacityMap.a)) < ALPHA_DISCARD_THRESHOLD) discard;
#elif defined(USE_OPACITY_DIFFUSE_MAP)
    if (texture(texDiffuse0, _texCoord).a < ALPHA_DISCARD_THRESHOLD) discard;
#endif

    // Adjusting moments (this is sort of bias per pixel) using partial derivative
    float linearz = clamp(gl_FragCoord.z*gl_FragCoord.w, 0, 1);
    //_colorOut = computeMoments(exp(DEPTH_EXP_WARP * linearz));
    _colorOut = computeMoments( linearz);
}

-- Vertex.PrePass

#include "vbInputData.vert"

void main() {

    computeData();    gl_Position = dvd_WorldViewProjectionMatrix * dvd_Vertex;}

-- Fragment.PrePass

in vec2 _texCoord;
in vec4 _vertexW;

uniform float opacity = 1.0;
uniform sampler2D texOpacityMap;
uniform sampler2D texDiffuse0;
uniform mat4      material;

out vec4 _colorOut;

void main(){

#if defined(USE_OPACITY_DIFFUSE)
    if (material[1].w < ALPHA_DISCARD_THRESHOLD) discard;
#elif defined(USE_OPACITY)
    if (opacity < ALPHA_DISCARD_THRESHOLD) discard;
#elif defined(USE_OPACITY_MAP)
    vec4 opacityMap = texture(texOpacityMap, _texCoord);
    if (max(min(opacityMap.r, opacityMap.g), min(opacityMap.b, opacityMap.a)) < ALPHA_DISCARD_THRESHOLD) discard;
#elif defined(USE_OPACITY_DIFFUSE_MAP)
    if (texture(texDiffuse0, _texCoord).a < ALPHA_DISCARD_THRESHOLD) discard;
#endif

    _colorOut = vec4(gl_FragCoord.w, 0.0, 0.0, 0.0);
}