#ifndef _BONE_TRANSFORM_VERT_
#define _BONE_TRANSFORM_VERT_

#if defined(HAS_VELOCITY)
layout(binding = BUFFER_BONE_TRANSFORMS_PREV, std140) uniform dvd_BoneTransformsPrev
{
    mat4 boneTransformsPrev[MAX_BONE_COUNT_PER_NODE];
};

void applyPrevBoneTransforms() {
    const mat4 transformMatrix[4] = mat4[](
        inBoneWeightData.x * boneTransformsPrev[inBoneIndiceData.x],
        inBoneWeightData.y * boneTransformsPrev[inBoneIndiceData.y],
        inBoneWeightData.z * boneTransformsPrev[inBoneIndiceData.z],
        inBoneWeightData.w * boneTransformsPrev[inBoneIndiceData.w]
    );

    dvd_PrevVertex = (transformMatrix[0] * dvd_PrevVertex + transformMatrix[1] * dvd_PrevVertex +
                      transformMatrix[2] * dvd_PrevVertex + transformMatrix[3] * dvd_PrevVertex);
}
#endif //HAS_VELOCITY

layout(binding = BUFFER_BONE_TRANSFORMS, std140) uniform dvd_BoneTransforms {
    mat4 boneTransforms[MAX_BONE_COUNT_PER_NODE];
};

void applyBoneTransforms() {
    const mat4 transformMatrix[4] = mat4[](
        inBoneWeightData.x * boneTransforms[inBoneIndiceData.x],
        inBoneWeightData.y * boneTransforms[inBoneIndiceData.y],
        inBoneWeightData.z * boneTransforms[inBoneIndiceData.z],
        inBoneWeightData.w * boneTransforms[inBoneIndiceData.w]
    );

    dvd_Vertex = (transformMatrix[0] * dvd_Vertex + transformMatrix[1] * dvd_Vertex +
                  transformMatrix[2] * dvd_Vertex + transformMatrix[3] * dvd_Vertex);

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
}
#endif //_BONE_TRANSFORM_VERT_