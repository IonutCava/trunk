#ifndef _BONE_TRANSFORM_VERT_
#define _BONE_TRANSFORM_VERT_

layout(binding = BUFFER_BONE_TRANSFORMS, std140) uniform dvd_BoneTransforms
{
    mat4 boneTransforms[MAX_BONE_COUNT_PER_NODE];
};

#if defined(HAS_VELOCITY)
layout(binding = BUFFER_BONE_TRANSFORMS_PREV, std140) uniform dvd_BoneTransformsPrev
{
    mat4 boneTransformsPrev[MAX_BONE_COUNT_PER_NODE];
};

mat4[4] transformMatricesPrev() {
    return mat4[](inBoneWeightData.x * boneTransformsPrev[inBoneIndiceData.x],
                  inBoneWeightData.y * boneTransformsPrev[inBoneIndiceData.y],
                  inBoneWeightData.z * boneTransformsPrev[inBoneIndiceData.z],
                  inBoneWeightData.w * boneTransformsPrev[inBoneIndiceData.w]);
}
#endif

mat4[4] transformMatrices() {
    return mat4[](inBoneWeightData.x * boneTransforms[inBoneIndiceData.x],
                  inBoneWeightData.y * boneTransforms[inBoneIndiceData.y],
                  inBoneWeightData.z * boneTransforms[inBoneIndiceData.z],
                  inBoneWeightData.w * boneTransforms[inBoneIndiceData.w]);
} //HAS_VELOCITY

void applyBoneTransforms(inout vec4 position, inout vec3 normal, inout vec3 tangnet, in int lod) {
    const mat4 transformMatrix[4] = transformMatrices();

    position = (transformMatrix[0] * position + transformMatrix[1] * position +
                transformMatrix[2] * position + transformMatrix[3] * position);

    if (lod >= 2) {
        return;
    }

    vec4 tempVec = vec4(normal, 0.0f);
    normal = vec4(transformMatrix[0] * tempVec + transformMatrix[1] * tempVec +
                  transformMatrix[2] * tempVec + transformMatrix[3] * tempVec).xyz;

    tempVec = vec4(tangnet, 0.0f);
    tangnet = vec4(transformMatrix[0] * tempVec + transformMatrix[1] * tempVec +
                   transformMatrix[2] * tempVec + transformMatrix[3] * tempVec).xyz;
}

void applyBoneTransforms(inout vec4 position, inout vec3 normal, in int lod) {
    const mat4 transformMatrix[4] = transformMatrices();

    position = (transformMatrix[0] * position + transformMatrix[1] * position +
                transformMatrix[2] * position + transformMatrix[3] * position);

    if (lod >= 2) {
        return;
    }

    vec4 tempVec = vec4(normal, 0.0f);
    normal = vec4(transformMatrix[0] * tempVec + transformMatrix[1] * tempVec +
                  transformMatrix[2] * tempVec + transformMatrix[3] * tempVec).xyz;
}

void applyBoneTransforms(inout vec4 position) {

    const mat4 transformMatrix[4] = transformMatrices();

    position = (transformMatrix[0] * position + transformMatrix[1] * position + 
                transformMatrix[2] * position + transformMatrix[3] * position);
}

#if defined(HAS_VELOCITY)
void applyPrevBoneTransforms(in uint boneCount, inout vec4 position) {
    if (boneCount == 0) {
        return;
    }

    const mat4 transformMatrix[4] = transformMatricesPrev();

    position = (transformMatrix[0] * position + transformMatrix[1] * position +
                transformMatrix[2] * position + transformMatrix[3] * position);
}
#endif //HAS_VELOCITY


void applyBoneTransforms(in uint boneCount, in int LoDLevel) {
    if (boneCount == 0) {
        return;
    }

#if !defined(SHADOW_PASS)

#if defined(COMPUTE_TBN) 
    applyBoneTransforms(dvd_Vertex, dvd_Normal, dvd_Tangent, LoDLevel);
#else //COMPUTE_TBN
    applyBoneTransforms(dvd_Vertex, dvd_Normal, LoDLevel);
#endif //COMUTE_TBN

#else //SHADOW_PASS
    applyBoneTransforms(dvd_Vertex);
#endif //SHADOW_PASS
}
#endif //_BONE_TRANSFORM_VERT_