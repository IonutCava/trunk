-- Vertex
uniform float time;

uniform mat4 modelViewInvMatrix;
//uniform mat4 modelViewProjectionMatrix;
//uniform mat4 transformMatrix;
//uniform mat4 parentTransformMatrix;

#include "vboInputData.vert"
#include "lightingDefaults.vert"

void main(void){

	computeData();

	//Compute the final vert position
	gl_Position = gl_ModelViewProjectionMatrix * vertexData;
	vec4 normal = vec4(normalize(gl_NormalMatrix * normalData),0);
	computeLightVectors();
}

-- Vertex.WithBones

uniform float time;

uniform mat4 modelViewInvMatrix;
//uniform mat4 modelViewProjectionMatrix;
//uniform mat4 transformMatrix;
//uniform mat4 parentTransformMatrix;
#include "boneTransforms.vert"
#include "lightingDefaults.vert"

void main(void){

	computeData();

	applyBoneTransforms(vertexData,normalData);
	//Compute the final vert position
	gl_Position = gl_ModelViewProjectionMatrix * vertexData;
	vec4 normal = vec4(normalize(gl_NormalMatrix * normalData),0);
	computeLightVectors();
}

-- Fragment.NoTexture

#include "phong_lighting_noTexture.frag"
#include "fog.frag"

void main (void){

	gl_FragDepth = gl_FragCoord.z;
	
	vec4 color = Phong(vNormalMV, vPixToEyeTBN, vPixToLightTBN[0]);

	gl_FragData[0] = applyFog(color);
}

-- Fragment.Texture

#include "phong_lighting.frag"
#include "fog.frag"

void main (void){

	gl_FragDepth = gl_FragCoord.z;

	vec4 color = Phong(texCoord[0].st, vNormalMV, vPixToEyeTBN, vPixToLightTBN[0]);
	
	gl_FragData[0] = applyFog(color);
}
	
-- Fragment.Bump

#include "phong_lighting.frag"
#include "bumpMapping.frag"
#include "fog.frag"

void main (void){

	gl_FragDepth = gl_FragCoord.z;

	vec4 color;
	//Else, use appropriate lighting / bump mapping / relief mapping / normal mapping
	if(mode == MODE_PHONG)
		color = Phong(texCoord[0].st, vNormalMV, vPixToEyeTBN, vPixToLightTBN[0]);
	else if(mode == MODE_RELIEF)
		color = ReliefMapping(texCoord[0].st);
	else if(mode == MODE_BUMP)
		color = NormalMapping(texCoord[0].st, vPixToEyeTBN, vPixToLightTBN[0], false);
	else if(mode == MODE_PARALLAX)
		color = NormalMapping(texCoord[0].st, vPixToEyeTBN, vPixToLightTBN[0], true);
	
	if(color.a < 0.2) discard;	
	
	gl_FragData[0] = applyFog(color);
}