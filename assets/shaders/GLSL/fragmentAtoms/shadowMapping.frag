uniform bool shadowPass;
// SHADOW MAPPING //
// 0->no  1->shadow mapping  2->shadow mapping + projected texture
uniform int enable_shadow_mapping;
//tdmfl0 -> high detail
//tdmfl1 -> medium detail
//tdmfl2 -> low detail
//change according to distance from eye
uniform float resolutionFactor;
uniform sampler2DShadow texDepthMapFromLight0;
uniform sampler2DShadow texDepthMapFromLight1;
uniform sampler2DShadow texDepthMapFromLight2;
uniform sampler2D texDiffuseProjected;
#define Z_TEST_SIGMA 0.0001
////////////////////

void applyShadow(inout vec4 cDiffuse, inout vec4 cAmbient, inout vec4 cSpecular, inout vec4 diffuseTexture);

float filterFinalShadow(sampler2DShadow depthMap,vec3 vPosInDM, float resolution);

float ShadowMapping(out vec3 vPixPosInDepthMap){

	float fShadow = 1.0;
			
	float tOrtho[3];
	tOrtho[0] = 5.0;
	tOrtho[1] = 10.0;
	tOrtho[2] = 50.0;
	bool ok = false;
	int id = 0;
	vec3 posInDM;
	for(int i = 0; i < 3; i++){
		if(!ok){
		
			vPixPosInDepthMap = vec3(texCoord[1].xy/tOrtho[i], texCoord[1].z) / (texCoord[1].w);
			vPixPosInDepthMap = (vPixPosInDepthMap + 1.0) * 0.5;

			if(vPixPosInDepthMap.x >= 0.0 && vPixPosInDepthMap.y >= 0.0 && vPixPosInDepthMap.x <= 1.0 && vPixPosInDepthMap.y <= 1.0){
				id = i;
				ok = true;
				posInDM = vPixPosInDepthMap;
				break; // no need to continue
			}
		}
	}

	if(ok){
		switch(id){
			case 0:
				fShadow = filterFinalShadow(texDepthMapFromLight0,posInDM, 2048/resolutionFactor);
				break;
			case 1:
				fShadow = filterFinalShadow(texDepthMapFromLight1,posInDM, 1024/resolutionFactor);
				break;
			case 2:
				fShadow = filterFinalShadow(texDepthMapFromLight2,posInDM, 512/resolutionFactor);
				break;
		};
	}
	return fShadow;
}

float filterFinalShadow(sampler2DShadow depthMap,vec3 vPosInDM, float resolution){
	// Gaussian 3x3 filter
	vec4 vDepthMapColor = shadow2D(depthMap, vPosInDM);
	float fShadow = 0.0;
	if((vDepthMapColor.z+Z_TEST_SIGMA) < vPosInDM.z){
		fShadow = shadow2D(depthMap, vPosInDM).x * 0.25;
		fShadow += shadow2DOffset(depthMap, vPosInDM, ivec2( -1, -1)).x * 0.0625;
		fShadow += shadow2DOffset(depthMap, vPosInDM, ivec2( -1, 0)).x * 0.125;
		fShadow += shadow2DOffset(depthMap, vPosInDM, ivec2( -1, 1)).x * 0.0625;
		fShadow += shadow2DOffset(depthMap, vPosInDM, ivec2( 0, -1)).x * 0.125;
		fShadow += shadow2DOffset(depthMap, vPosInDM, ivec2( 0, 1)).x * 0.125;
		fShadow += shadow2DOffset(depthMap, vPosInDM, ivec2( 1, -1)).x * 0.0625;
		fShadow += shadow2DOffset(depthMap, vPosInDM, ivec2( 1, 0)).x * 0.125;
		fShadow += shadow2DOffset(depthMap, vPosInDM, ivec2( 1, 1)).x * 0.0625;

		fShadow = clamp(fShadow, 0.0, 1.0);
	}else{
		fShadow = 1.0;
	}
	return fShadow;
}

void applyShadow(inout vec4 cDiffuse, inout vec4 cAmbient, inout vec4 cSpecular, inout vec4 diffuseTexture) {
	float shadow = 1.0;
	if(enable_shadow_mapping != 0) {
		/////////////////////////
		// SHADOW MAPS
		vec3 vPixPosInDepthMap;
		//Compute shadow value for current fragment
		shadow = ShadowMapping(vPixPosInDepthMap);
		//And add shadow value to current diffuse color and specular values
		cDiffuse = (shadow) * cDiffuse;
		cSpecular = (shadow) * cSpecular;
		cAmbient = (shadow) * cAmbient;
		// Texture projection :
		if(enable_shadow_mapping == 2) {
			vec4 cProjected = texture2D(texDiffuseProjected, vec2(vPixPosInDepthMap.s, 1.0-vPixPosInDepthMap.t));
			diffuseTexture.xyz = mix(diffuseTexture.xyz, cProjected.xyz, shadow/2.0);
		}
	}
}