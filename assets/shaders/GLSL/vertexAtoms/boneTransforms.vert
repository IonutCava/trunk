
uniform vec4 boneWeightValue;
uniform vec4 boneWeightIndex;
uniform mat4 boneMatrices[32];

void applyBoneTransforms(inout vec4 position){
	vec4 pos1 = boneWeightValue.x * (boneMatrices[int(boneWeightIndex.x)] * position);
	vec4 pos2 = boneWeightValue.y * (boneMatrices[int(boneWeightIndex.y)] * position);
	vec4 pos3 = boneWeightValue.z * (boneMatrices[int(boneWeightIndex.z)] * position);
	vec4 pos4 = boneWeightValue.w * (boneMatrices[int(boneWeightIndex.w)] * position);
	vec4 finalPosition =  pos1 + pos2 + pos3 + pos4;
	//Uncomment the following line to enable skeletal animation
	//position = finalPosition;

}