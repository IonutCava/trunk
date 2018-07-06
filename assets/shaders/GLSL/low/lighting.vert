varying vec2 texCoord;

uniform int enable_shadow_mapping;

uniform vec4 boneWeightValue;
uniform vec4 boneWeightIndex;
uniform mat4 boneMatrices[32];

uniform mat4 modelViewInvMatrix;
uniform mat4 lightProjectionMatrix;
uniform mat4 modelViewProjectionMatrix;

#define LIGHT_DIRECTIONAL		0.0
#define LIGHT_OMNIDIRECTIONAL	1.0
#define LIGHT_SPOT				2.0
				
void main(void){

	vec4 position = gl_Vertex;
	vec4 pos1 = boneWeightValue.x * (boneMatrices[int(boneWeightIndex.x)] * position);
	vec4 pos2 = boneWeightValue.y * (boneMatrices[int(boneWeightIndex.y)] * position);
	vec4 pos3 = boneWeightValue.z * (boneMatrices[int(boneWeightIndex.z)] * position);
	vec4 pos4 = boneWeightValue.w * (boneMatrices[int(boneWeightIndex.w)] * position);

	vec4 finalPosition =  pos1 + pos2 + pos3 + pos4;
	//Comment the following line to enable skeletal animation
	finalPosition = position;
	gl_Position = gl_ModelViewProjectionMatrix * finalPosition;

	texCoord = gl_MultiTexCoord0.xy;
}
