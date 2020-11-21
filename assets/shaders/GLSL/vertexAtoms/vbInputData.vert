#ifndef _VB_INPUT_DATA_VERT_
#define _VB_INPUT_DATA_VERT_

#include "vertexDefault.vert"
#include "nodeBufferedInput.cmn"

vec4   dvd_Vertex;
#if defined(HAS_VELOCITY)
vec4  dvd_PrevVertex;
#endif
#if !defined(SHADOW_PASS)
vec3   dvd_Normal;
#endif //SHADOW_PASS
#if defined(COMPUTE_TBN)
vec3   dvd_Tangent;
#endif //COMPUTE_TBN
#if !defined(DEPTH_PASS)
vec4   dvd_Colour;
#endif //DEPTH_PASS

#if defined(USE_GPU_SKINNING)
#include "boneTransforms.vert"
#endif // USE_GPU_SKINNING

NodeData fetchInputData() {
    dvd_Vertex = vec4(inVertexData, 1.0);

#if !defined(USE_MIN_SHADING)

#if defined(HAS_VELOCITY)
    dvd_PrevVertex = dvd_Vertex;
#endif //HAS_VELOCITY

#if !defined(SHADOW_PASS)
    dvd_Normal = UNPACK_VEC3(inNormalData);
#endif //SHADOW_PASS

#if !defined(DEPTH_PASS)
    dvd_Colour = inColourData;
#endif //DEPTH_PASS

#if defined(COMPUTE_TBN)
    dvd_Tangent = UNPACK_VEC3(inTangentData);
#endif //COMPUTE_TBN

    VAR._texCoord = inTexCoordData;
#endif //USE_MIN_SHADING

    VAR._baseInstance = DVD_GL_BASE_INSTANCE;

    return dvd_Matrices[DATA_IDX];
}

void computeData(in NodeData data) {
#if defined(USE_GPU_SKINNING)
    if (dvd_boneCount(data) > 0) {
        applyBoneTransforms();
#if defined(HAS_VELOCITY)
        if (dvd_frameTicked(data)) {
            applyPrevBoneTransforms();
        } else {
            dvd_PrevVertex = dvd_Vertex;
        }
#endif //HAS_VELOCITY
    }
#endif //USE_GPU_SKINNING

    VAR._vertexW = data._worldMatrix * dvd_Vertex;
    VAR._vertexWV = dvd_ViewMatrix * VAR._vertexW;
    VAR._vertexWVP = dvd_ProjectionMatrix * VAR._vertexWV;

#if defined(HAS_VELOCITY)
    dvd_PrevVertex = vec4(mat4x3(data._prevWorldMatrix) * dvd_PrevVertex, 1.0f);
#if defined(USE_CAMERA_BLUR)
    VAR._prevVertexWVP = dvd_PreviousViewProjectionMatrix * dvd_PrevVertex;
#else //USE_CAMERA_BLUR
    VAR._prevVertexWVP = dvd_ViewProjectionMatrix * dvd_PrevVertex;
#endif //USE_CAMERA_BLUR
#endif //HAS_VELOCITY
}

#endif //_VB_INPUT_DATA_VERT_