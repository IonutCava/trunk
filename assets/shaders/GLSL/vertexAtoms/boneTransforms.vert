
uniform bool dvd_hasAnimations = false;
uniform mat4 boneTransforms[40];

void applyBoneTransforms(inout vec4 position, inout vec3 normal, in int lod){
    if (!dvd_hasAnimations)
        return;
    
    //w - weight value
    float boneWeightW = 1.0 - dot(inBoneWeightData.xyz, vec3(1.0, 1.0, 1.0));

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