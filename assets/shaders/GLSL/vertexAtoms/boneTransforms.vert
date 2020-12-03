#ifndef _BONE_TRANSFORM_VERT_
#define _BONE_TRANSFORM_VERT_

#if defined(HAS_VELOCITY)
layout(binding = BUFFER_BONE_TRANSFORMS_PREV, std140) uniform dvd_BoneTransformsPrev
{
    mat4 boneTransformsPrev[MAX_BONE_COUNT_PER_NODE];
};

vec4 applyPrevBoneTransforms(in vec4 vertex) {
    const mat4 transformMatrix[4] = mat4[](
        inBoneWeightData.x * boneTransformsPrev[inBoneIndiceData.x],
        inBoneWeightData.y * boneTransformsPrev[inBoneIndiceData.y],
        inBoneWeightData.z * boneTransformsPrev[inBoneIndiceData.z],
        inBoneWeightData.w * boneTransformsPrev[inBoneIndiceData.w]
    );

    return (transformMatrix[0] * vertex + transformMatrix[1] * vertex +
            transformMatrix[2] * vertex + transformMatrix[3] * vertex);
}
#endif //HAS_VELOCITY

layout(binding = BUFFER_BONE_TRANSFORMS, std140) uniform dvd_BoneTransforms {
    mat4 boneTransforms[MAX_BONE_COUNT_PER_NODE];
};

vec4 applyBoneTransforms(in vec4 vertex) {
    const mat4 transformMatrix[4] = mat4[](
        inBoneWeightData.x * boneTransforms[inBoneIndiceData.x],
        inBoneWeightData.y * boneTransforms[inBoneIndiceData.y],
        inBoneWeightData.z * boneTransforms[inBoneIndiceData.z],
        inBoneWeightData.w * boneTransforms[inBoneIndiceData.w]
    );

    const vec4 ret = (transformMatrix[0] * vertex + transformMatrix[1] * vertex +
                      transformMatrix[2] * vertex + transformMatrix[3] * vertex);

#if !defined(SHADOW_PASS)
    vec4 tempNormal = vec4(dvd_Normal, 0.0f);
    dvd_Normal = vec4(transformMatrix[0] * tempNormal + transformMatrix[1] * tempNormal +
                      transformMatrix[2] * tempNormal + transformMatrix[3] * tempNormal).xyz;
#if defined(COMPUTE_TBN) 
    vec4 tempTangent = vec4(dvd_Tangent, 0.0f);
    dvd_Tangent = vec4(transformMatrix[0] * tempTangent + transformMatrix[1] * tempTangent +
                       transformMatrix[2] * tempTangent + transformMatrix[3] * tempTangent).xyz;
#endif //COMPUTE_TBN

#endif //SHADOW_PASS

    return ret;
}
#endif //_BONE_TRANSFORM_VERT_