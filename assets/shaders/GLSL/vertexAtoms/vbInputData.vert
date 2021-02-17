#ifndef _VB_INPUT_DATA_VERT_
#define _VB_INPUT_DATA_VERT_

#include "velocityCheck.cmn"
#include "vertexDefault.vert"
#include "nodeBufferedInput.cmn"
#if defined(USE_GPU_SKINNING)
#include "boneTransforms.vert"
#endif // USE_GPU_SKINNING

NodeTransformData fetchInputData() {
    VAR._baseInstance = DVD_GL_BASE_INSTANCE;
    VAR._texCoord = inTexCoordData;

    dvd_Vertex = vec4(inVertexData, 1.f);

#if !defined(DEPTH_PASS)
    dvd_Normal = UNPACK_VEC3(inNormalData);
    dvd_Colour = inColourData;
#if defined(COMPUTE_TBN) || defined(NEED_TANGENT)
    dvd_Tangent = UNPACK_VEC3(inTangentData);
#endif //COMPUTE_TBN || NEED_TANGENT
#endif //DEPTH_PASS

#if defined(USE_GPU_SKINNING)
    const NodeTransformData nodeData = dvd_Transforms[TRANSFORM_IDX];
    if (dvd_boneCount(nodeData) > 0) {
        dvd_Vertex = applyBoneTransforms(dvd_Vertex);
    }
    return nodeData;
#else //USE_GPU_SKINNING
    return dvd_Transforms[TRANSFORM_IDX];
#endif //USE_GPU_SKINNING
}

vec4 computeData(in NodeTransformData data) {
    VAR._vertexW   = data._worldMatrix * dvd_Vertex;
    VAR._vertexWV  = dvd_ViewMatrix * VAR._vertexW;
    vec4 vertexWVP = dvd_ProjectionMatrix * VAR._vertexWV;

#if !defined(SHADOW_PASS) && defined(HAS_VELOCITY)
#if defined(USE_GPU_SKINNING)
    const vec4 prevVertex = (dvd_frameTicked(data) && dvd_boneCount(data) > 0) ? applyPrevBoneTransforms(dvd_Vertex) : dvd_Vertex;
#else //USE_GPU_SKINNING
    const vec4 prevVertex = dvd_Vertex;
#endif //USE_GPU_SKINNING

    VAR._prevVertexWVP_XYW = (dvd_PreviousViewProjectionMatrix * vec4(mat4x3(data._prevWorldMatrix) * prevVertex, 1.0f)).xyw;
#endif //!SHADOW_PASS && HAS_VELOCITY

    return vertexWVP;
}

#endif //_VB_INPUT_DATA_VERT_
