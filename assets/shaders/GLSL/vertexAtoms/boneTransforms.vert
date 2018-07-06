layout(binding = SHADER_BUFFER_BONE_TRANSFORMS, std140) uniform dvd_BoneTransforms
{
    mat4 boneTransforms[MAX_BONE_COUNT_PER_NODE];
};

void applyBoneTransforms(inout vec4 position, inout vec3 normal, in int lod){
    if(dvd_boneCount == 0)
        return;

    //w - weight value
    float boneWeightW = 1.0 - dot(inBoneWeightData.xyz, vec3(1.0));
    vec4 newPosition  = inBoneWeightData.x * boneTransforms[inBoneIndiceData.x] * position;
         newPosition += inBoneWeightData.y * boneTransforms[inBoneIndiceData.y] * position;
         newPosition += inBoneWeightData.z * boneTransforms[inBoneIndiceData.z] * position;
         newPosition += boneWeightW        * boneTransforms[inBoneIndiceData.w] * position;
         
    position = newPosition;
    
    if (lod >= 2)
        return;

    vec4 newNormalT = vec4(normal,0.0);
    vec4 newNormal  = inBoneWeightData.x * boneTransforms[inBoneIndiceData.x] * newNormalT;
         newNormal += inBoneWeightData.y * boneTransforms[inBoneIndiceData.y] * newNormalT;
         newNormal += inBoneWeightData.z * boneTransforms[inBoneIndiceData.z] * newNormalT;
         newNormal += boneWeightW        * boneTransforms[inBoneIndiceData.w] * newNormalT;
      
    normal = newNormal.xyz;
}