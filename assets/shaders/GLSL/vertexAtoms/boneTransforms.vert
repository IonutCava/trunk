#include "vboInputDataBones.vert"

uniform bool hasAnimations;
uniform mat4 boneTransforms[60];

void applyBoneTransforms(inout vec4 position, inout vec3 normal){
	if(hasAnimations) {

	 // ///w - weight value
	  boneWeightData.w = 1.0 - dot(boneWeightData.xyz, vec3(1.0, 1.0, 1.0));

	   vec4 newPosition  = boneWeightData.x * boneTransforms[boneIndiceData.x] * position;
		    newPosition += boneWeightData.y * boneTransforms[boneIndiceData.y] * position;
		    newPosition += boneWeightData.z * boneTransforms[boneIndiceData.z] * position;
		    newPosition += boneWeightData.w * boneTransforms[boneIndiceData.w] * position;

	  position = newPosition;

	  vec4 newNormalT = vec4(normal,0.0);
	  vec4 newNormal  = boneWeightData.x * boneTransforms[boneIndiceData.x] * newNormalT;
		   newNormal += boneWeightData.y * boneTransforms[boneIndiceData.y] * newNormalT;
		   newNormal += boneWeightData.z * boneTransforms[boneIndiceData.z] * newNormalT;
		   newNormal += boneWeightData.w * boneTransforms[boneIndiceData.w] * newNormalT;
	   normal = newNormal.xyz;

	}
}