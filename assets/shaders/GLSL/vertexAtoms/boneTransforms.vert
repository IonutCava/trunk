uniform bool hasAnimations;
uniform mat4 boneTransforms[60];

void applyBoneTransforms(inout vec4 position, inout vec4 normal){
	if(hasAnimations) {
		vec4 boneWeightIndex = gl_MultiTexCoord3;
		vec4 boneWeightValue = gl_MultiTexCoord4;

		 mat4 matTransform = boneTransforms[int(boneWeightIndex.x)] * boneWeightValue.x;
		 matTransform += boneTransforms[int(boneWeightIndex.y)] * boneWeightValue.y;
		 matTransform += boneTransforms[int(boneWeightIndex.z)] * boneWeightValue.z;
		 float finalWeight = 1.0f - ( boneWeightValue.x + boneWeightValue.y + boneWeightValue.z );
		 matTransform += boneTransforms[int(boneWeightIndex.w)] * finalWeight;
 
         vec4 objPos = mul( matTransform, vec4(position.xyz,1.0));
         vec4 objNormal = mul( matTransform, vec4(normal.xyz,0.0));

		 //position = objPos;
		 //normal = objNormal;
	}
}
