#ifndef _BONE_TRANSFORM_VERT_
#define _BONE_TRANSFORM_VERT_

layout(binding = BUFFER_BONE_TRANSFORMS, std140) uniform dvd_BoneTransforms
{
    mat4 boneTransforms[MAX_BONE_COUNT_PER_NODE];
};

#if defined(COMPUTE_TBN)
void applyBoneTransforms(inout vec4 position, inout vec3 normal, inout vec3 tangnet, in int lod){
#else
void applyBoneTransforms(inout vec4 position, inout vec3 normal, in int lod){
#endif
    if (dvd_boneCount == 0) {
        return;
    }
    mat4 transformMatrix[4] = mat4[]( inBoneWeightData.x * boneTransforms[inBoneIndiceData.x],
                                      inBoneWeightData.y * boneTransforms[inBoneIndiceData.y],
                                      inBoneWeightData.z * boneTransforms[inBoneIndiceData.z],
                                      (1.0 - dot(inBoneWeightData.xyz, vec3(1.0))) * boneTransforms[inBoneIndiceData.w] );

    position = (transformMatrix[0] * position + transformMatrix[1] * position + 
                transformMatrix[2] * position + transformMatrix[3] * position);
    
    if (lod >= 2)
        return;

    vec4 tempVec = vec4(normal, 0.0);
    normal = vec4(transformMatrix[0] * tempVec + transformMatrix[1] * tempVec + 
                  transformMatrix[2] * tempVec + transformMatrix[3] * tempVec).xyz;

#if defined(COMPUTE_TBN)
    tempVec = vec4(tangnet, 0.0);
    tangnet = vec4(transformMatrix[0] * tempVec + transformMatrix[1] * tempVec + 
                   transformMatrix[2] * tempVec + transformMatrix[3] * tempVec).xyz;
#endif
    
}

#endif //_BONE_TRANSFORM_VERT_