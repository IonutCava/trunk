#ifndef _VB_INPUT_DATA_VERT_
#define _VB_INPUT_DATA_VERT_

#include "velocityCheck.cmn"

#include "vertexDefault.vert"
#include "nodeBufferedInput.cmn"

vec4   dvd_Vertex;

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

NodeTransformData fetchInputData() {
    VAR._baseInstance = DVD_GL_BASE_INSTANCE;
    NodeTransformData data = dvd_Transforms[TRANSFORM_IDX];

#if !defined(USE_MIN_SHADING)

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

#if defined(USE_GPU_SKINNING)
    dvd_Vertex = dvd_boneCount(data) > 0 ? applyBoneTransforms(vec4(inVertexData, 1.0)) : vec4(inVertexData, 1.0);
#else //USE_GPU_SKINNING
    dvd_Vertex = vec4(inVertexData, 1.0);
#endif //USE_GPU_SKINNING

    return data;
}

vec4 computeData(in NodeTransformData data) {
    VAR._vertexW = data._worldMatrix * dvd_Vertex;
    VAR._vertexWV = dvd_ViewMatrix * VAR._vertexW;
    vec4 vertexWVP = dvd_ProjectionMatrix * VAR._vertexWV;

#if defined(HAS_VELOCITY)
#if defined(USE_GPU_SKINNING)
    const vec4 prevVertex = (dvd_frameTicked(data) && dvd_boneCount(data) > 0) ? applyPrevBoneTransforms(dvd_Vertex) : dvd_Vertex;
#else //USE_GPU_SKINNING
    const vec4 prevVertex = dvd_Vertex;
#endif //USE_GPU_SKINNING

    VAR._prevVertexWVP_XYW = (dvd_PreviousViewProjectionMatrix * vec4(mat4x3(data._prevWorldMatrix) * prevVertex, 1.0f)).xyw;
#endif //HAS_VELOCITY

    return vertexWVP;
}

#endif //_VB_INPUT_DATA_VERT_
