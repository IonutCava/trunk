
float filterFinalShadow(sampler2DShadow depthMap,vec3 vPosInDM){
	// Gaussian 3x3 filter
	float vDepthMapColor = texture(depthMap, vPosInDM);
	float fShadow = 0.0;
	float Z_TEST_SIGMA = 0.0001f;
	if((vDepthMapColor+Z_TEST_SIGMA) < vPosInDM.z){
		fShadow = vDepthMapColor * 0.25;
		fShadow += textureOffset(depthMap, vPosInDM, ivec2( -1, -1)) * 0.0625;
		fShadow += textureOffset(depthMap, vPosInDM, ivec2( -1, 0)) * 0.125;
		fShadow += textureOffset(depthMap, vPosInDM, ivec2( -1, 1)) * 0.0625;
		fShadow += textureOffset(depthMap, vPosInDM, ivec2( 0, -1)) * 0.125;
		fShadow += textureOffset(depthMap, vPosInDM, ivec2( 0, 1)) * 0.125;
		fShadow += textureOffset(depthMap, vPosInDM, ivec2( 1, -1)) * 0.0625;
		fShadow += textureOffset(depthMap, vPosInDM, ivec2( 1, 0)) * 0.125;
		fShadow += textureOffset(depthMap, vPosInDM, ivec2( 1, 1)) * 0.0625;

		fShadow = clamp(fShadow, 0.0, 1.0);
	}else{
		fShadow = 1.0;
	}
	return fShadow;
}

void applyShadowSpot(in int _shadowIndex, inout float shadow) {
	///Limit maximum number of lights that cast shadows
	if(!enable_shadow_mapping) return;

	// SHADOW MAPS
	vec3 vPixPosInDepthMap;

	vPixPosInDepthMap = shadowCoord[_shadowIndex].xyz/shadowCoord[_shadowIndex].w;
	vPixPosInDepthMap = (vPixPosInDepthMap + 1.0) * 0.5;	
	vec4 vDepthMapColor;
	switch(_shadowIndex){
		default:
		case 0:	shadow =  filterFinalShadow(texDepthMapFromLight0, vPixPosInDepthMap); break;
		case 1:	shadow =  filterFinalShadow(texDepthMapFromLight1, vPixPosInDepthMap); break;
		case 2: shadow =  filterFinalShadow(texDepthMapFromLight2, vPixPosInDepthMap); break;
		case 3: shadow =  filterFinalShadow(texDepthMapFromLight3, vPixPosInDepthMap); break;
	};
}