uniform bool dvd_hasAnimations = false;
uniform int dvd_boneOffset = 0;

// 256 bones is more than enough
layout(binding = SHADER_BUFFER_BONE_TRANSFORMS, std430) buffer dvd_BoneTransforms
{
    mat4 boneTransforms[];
};

void applyBoneTransforms(inout vec4 position, inout vec3 normal, in int lod){
    if (!dvd_hasAnimations)
        return;
    
    //w - weight value
    float boneWeightW = 1.0 - dot(inBoneWeightData.xyz, vec3(1.0));
	ivec4 boneIndices = inBoneIndiceData + ivec4(dvd_boneOffset);

    vec4 newPosition  = inBoneWeightData.x * boneTransforms[boneIndices.x] * position;
         newPosition += inBoneWeightData.y * boneTransforms[boneIndices.y] * position;
         newPosition += inBoneWeightData.z * boneTransforms[boneIndices.z] * position;
         newPosition += boneWeightW        * boneTransforms[boneIndices.w] * position;

    position = newPosition;

    if (lod >= 2)
        return;

    vec4 newNormalT = vec4(normal,0.0);
    vec4 newNormal  = inBoneWeightData.x * boneTransforms[boneIndices.x] * newNormalT;
         newNormal += inBoneWeightData.y * boneTransforms[boneIndices.y] * newNormalT;
         newNormal += inBoneWeightData.z * boneTransforms[boneIndices.z] * newNormalT;
         newNormal += boneWeightW        * boneTransforms[boneIndices.w] * newNormalT;
       
    normal = newNormal.xyz;

}