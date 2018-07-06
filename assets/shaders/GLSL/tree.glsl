-- Vertex
uniform float time;
uniform vec3  scale;
uniform float windDirectionX;
uniform float windDirectionZ;
uniform float windSpeed;

uniform mat4 modelViewInvMatrix;
uniform mat4 modelViewProjectionMatrix;
#include "vboInputData.vert"
#include "lightingDefaults.vert"

void main(void){
	computeData();
	vec4 position = vertexData;
	
	vec4 vertexM = gl_TextureMatrix[0] * gl_ModelViewMatrix * vec4(0.0, 0.0, 0.0, 1.0);
	position.x += 0.05 * cos(time*windSpeed) * cos(position.x) * sin(position.x) *windDirectionX;
	position.z += 0.05 * sin(time*windSpeed) * cos(position.x) * sin(position.x) *windDirectionZ;

	gl_Position = gl_ModelViewProjectionMatrix * position;
	
	computeLightVectors();
}

-- Fragment.Texture

#include "phong_lighting.frag"
#include "fog.frag"

void main (void){

	gl_FragDepth = gl_FragCoord.z;
	vec4 vPixToLightTBNcurrent = vPixToLightTBN[0];

	vec4 color = Phong(texCoord[0].st, vNormalMV, vPixToEyeTBN, vPixToLightTBNcurrent);
	
	gl_FragData[0] = applyFog(color);
}
	
-- Fragment.Bump

#include "phong_lighting.frag"
#include "bumpMapping.frag"
#include "fog.frag"

void main (void){

	gl_FragDepth = gl_FragCoord.z;
	vec4 vPixToLightTBNcurrent = vPixToLightTBN[0];

	vec4 color;
	//Else, use appropriate lighting / bump mapping / relief mapping / normal mapping
	if(mode == MODE_PHONG)
		color = Phong(texCoord[0].st, vNormalMV, vPixToEyeTBN, vPixToLightTBNcurrent);
	else if(mode == MODE_RELIEF)
		color = ReliefMapping(texCoord[0].st);
	else if(mode == MODE_BUMP)
		color = NormalMapping(texCoord[0].st, vPixToEyeTBN, vPixToLightTBNcurrent, false);
	else if(mode == MODE_PARALLAX)
		color = NormalMapping(texCoord[0].st, vPixToEyeTBN, vPixToLightTBNcurrent, true);
	
	gl_FragData[0] = applyFog(color);
}