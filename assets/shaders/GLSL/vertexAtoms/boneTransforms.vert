
uniform bool hasAnimations;
uniform mat4 boneTransforms[60];

void applyBoneTransforms(inout vec4 position, inout vec3 normal){
    if(!hasAnimations) return;

    computeBoneData();

    //w - weight value
    dvd_BoneWeight.w = 1.0 - dot(dvd_BoneWeight.xyz, vec3(1.0, 1.0, 1.0));

    vec4 newPosition  = dvd_BoneWeight.x * boneTransforms[dvd_BoneIndice.x] * position;
         newPosition += dvd_BoneWeight.y * boneTransforms[dvd_BoneIndice.y] * position;
         newPosition += dvd_BoneWeight.z * boneTransforms[dvd_BoneIndice.z] * position;
         newPosition += dvd_BoneWeight.w * boneTransforms[dvd_BoneIndice.w] * position;

    position = newPosition;

    vec4 newNormalT = vec4(normal,0.0);
    vec4 newNormal  = dvd_BoneWeight.x * boneTransforms[dvd_BoneIndice.x] * newNormalT;
         newNormal += dvd_BoneWeight.y * boneTransforms[dvd_BoneIndice.y] * newNormalT;
         newNormal += dvd_BoneWeight.z * boneTransforms[dvd_BoneIndice.z] * newNormalT;
         newNormal += dvd_BoneWeight.w * boneTransforms[dvd_BoneIndice.w] * newNormalT;
       
    normal = newNormal.xyz;

    
}