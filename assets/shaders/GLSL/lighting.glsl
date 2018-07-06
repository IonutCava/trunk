//-- Geometry

//#include "inOut.geom"

-- Vertex

#include "vboInputData.vert"
#include "lightingDefaults.vert"
#if defined(ADD_FOLIAGE)
#include "foliage.vert"
#endif
void main(void){

	computeData();
#if defined(ADD_FOLIAGE) && defined(IS_TREE)
	computeFoliageMovementTree(vertexData);
#endif

	computeLightVectors(false);
	//Compute the final vert position
	gl_Position = projectionMatrix * _vertexMV;
}

-- Vertex.Bump

#include "vboInputData.vert"
#include "lightingDefaults.vert"
#if defined(ADD_FOLIAGE)
#include "foliage.vert"
#endif

void main(void){

	computeData();
#if defined(ADD_FOLIAGE) && defined(IS_TREE)
	computeFoliageMovementTree(vertexData);
#endif
	computeLightVectors(true);
	//Compute the final vert position
	gl_Position = projectionMatrix  * _vertexMV;
}

-- Vertex.WithBones

#include "vboInputDataBones.vert"
#include "boneTransforms.vert"
#include "lightingDefaults.vert"

void main(void){

	computeData();
	applyBoneTransforms(vertexData,normalData);
	computeLightVectors(false);
	//Compute the final vert position
	gl_Position = projectionMatrix  * _vertexMV;
}

-- Vertex.WithBones.Bump

#include "vboInputData.vert"
#include "lightingDefaults.vert"

void main(void){

	computeData();

	applyBoneTransforms(vertexData,normalData);
	computeLightVectors(true);
	//Compute the final vert position
	gl_Position = projectionMatrix  * _vertexMV;
}

-- Fragment

#include "phong_lighting_noTexture.frag"
#include "fog.frag"

void main (void){
	gl_FragDepth = gl_FragCoord.z;

	vec4 color = Phong(_normalMV);
    applyFog(color);

	gl_FragData[0] = color;
}

-- Fragment.Texture

#include "phong_lighting.frag"
#include "fog.frag"

void main (void){
	gl_FragDepth = gl_FragCoord.z;

	vec4 color = Phong(_texCoord, _normalMV);
	applyFog(color);

	gl_FragData[0] = color;
}
	
-- Fragment.Bump

#include "phong_lighting.frag"
#include "bumpMapping.frag"
#include "fog.frag"

void main (void){

	gl_FragDepth = gl_FragCoord.z;

	vec4 color = NormalMapping(_texCoord);
	
	if(color.a < 0.2) discard;	
	applyFog(color);

	gl_FragData[0] = color;
}

-- Fragment.Bump.Parallax

#include "phong_lighting.frag"
#include "bumpMapping.frag"
#include "fog.frag"

void main (void){

	gl_FragDepth = gl_FragCoord.z;

	vec4 color = ParallaxMapping(_texCoord, vPixToEyeTBN[bumpMapLightId], vPixToLightTBN[bumpMapLightId]);
	
	if(color.a < 0.2) discard;	
	applyFog(color);

	gl_FragData[0] = color;
}

-- Fragment.Bump.Relief

#include "phong_lighting.frag"
#include "bumpMapping.frag"
#include "fog.frag"

void main (void){

	gl_FragDepth = gl_FragCoord.z;

	vec4 color = ReliefMapping(bumpMapLightId,_texCoord);

	if(color.a < 0.2) discard;	
	applyFog(color);

	gl_FragData[0] = color;
}