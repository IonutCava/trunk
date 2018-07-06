-- Vertex.Shadow

uniform mat4 dvd_WorldMatrix;//[MAX_INSTANCES];
#ifndef CSM_USE_LAYERED_RENDERING

out vec4 _vertexWVP;
#endif
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
out vec2 texCoord;

void main(void){
    vec4 dvd_Vertex = vec4(inVertexData, 1.0);

#if defined(USE_GPU_SKINNING)
    vec3 dvd_Normal = vec3(1.0);
    applyBoneTransforms(dvd_Vertex, dvd_Normal, 2);
#endif

    texCoord = inTexCoordData;
    vec4 wVertex = dvd_WorldMatrix * dvd_Vertex;
    //setClipPlanes(wVertex);
#ifdef CSM_USE_LAYERED_RENDERING
    gl_Position = wVertex;
#else
    gl_Position = dvd_ViewProjectionMatrix * wVertex;
    _vertexWVP = gl_Position;
#endif
}

-- Geometry.Shadow

precision highp int;

layout(triangles, invocations = MAX_SPLITS_PER_LIGHT) in;
layout(triangle_strip, max_vertices = 9) out;

uniform mat4 dvd_shadowCPV[MAX_SPLITS_PER_LIGHT]; //Should contain Crop * Projection * View
uniform int  dvd_currentSplitCount = 4;

in  vec2 texCoord[];
out vec2 _texCoord;
out vec4 _vertexWVP;

void main(){

    int layer = gl_InvocationID;
 
    if (layer == dvd_currentSplitCount) 
       return;

    gl_Layer = layer;
    mat4 shadowMatrix = dvd_shadowCPV[layer];
    for (int vertIdx = 0; vertIdx < 3; vertIdx++) {
        _texCoord = texCoord[vertIdx];
        gl_Position = shadowMatrix * gl_in[vertIdx].gl_Position;
        _vertexWVP = gl_Position;
        EmitVertex();
    }
        
    EndPrimitive();
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

in  vec4 _vertexWVP;
out float _colorOut;

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

    _colorOut = _vertexWVP.z / _vertexWVP.w;;
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