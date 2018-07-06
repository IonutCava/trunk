varying vec4 texCoord[2];
varying vec3 vVertexMV;

uniform sampler2D texDiffuse;

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
#define Z_TEST_SIGMA 0.0001
////////////////////

float ShadowMapping(out vec3 vPixPosInDepthMap);
float filterFinalShadow(sampler2DShadow depthMap,vec3 vPosInDM, float resolution);

void main (void)
{
	vec4 cBase = texture2D(texDiffuse, texCoord[0].st);
	if(cBase.a < 0.4) discard;
	
	vec4 cAmbient = gl_LightSource[0].ambient * gl_FrontMaterial.ambient;
	vec4 cDiffuse = gl_LightSource[0].diffuse * gl_FrontMaterial.diffuse * gl_Color;
	gl_FragColor = cAmbient * cBase + cDiffuse * cBase;
	
	// SHADOW MAPPING
	vec3 vPixPosInDepthMap;
	float shadow = ShadowMapping(vPixPosInDepthMap);
	shadow = shadow * 0.5 + 0.5;
	gl_FragColor *= shadow;

	
	gl_FragColor.a = gl_Color.a;
}


float ShadowMapping(out vec3 vPixPosInDepthMap){

	float fShadow = 1.0;
			
	float tOrtho[3];
	tOrtho[0] = 2.0;
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
	float fShadow = 0.0;
						
	vec2 tOffset[3*3];
	tOffset[0] = vec2(-1.0, -1.0); tOffset[1] = vec2(0.0, -1.0); tOffset[2] = vec2(1.0, -1.0);
	tOffset[3] = vec2(-1.0,  0.0); tOffset[4] = vec2(0.0,  0.0); tOffset[5] = vec2(1.0,  0.0);
	tOffset[6] = vec2(-1.0,  1.0); tOffset[7] = vec2(0.0,  1.0); tOffset[8] = vec2(1.0,  1.0);

	vec4 vDepthMapColor = shadow2D(depthMap, vPosInDM);
	
	if((vDepthMapColor.z+Z_TEST_SIGMA) < vPosInDM.z)
	{
		fShadow = 0.0;
		
		// Soft Shadows for nearby textures
		if( length(vVertexMV.xyz) < 15.0 )
		{
			for(int i=0; i<9; i++)
			{
				vec2 offset = tOffset[i] / resolution;
				// Couleur du pixel sur la depth map
				vec4 vDepthMapColor = shadow2D(depthMap, vPosInDM + vec3(offset.s, offset.t, 0.0));
		
				if((vDepthMapColor.z+Z_TEST_SIGMA) < vPosInDM.z) {
					fShadow += 0.0;
				}
				else {
					fShadow += 1.0 / 9.0;
				}
			}
		}
	}
	else
	{
		fShadow = 1.0;
	}

	fShadow = clamp(fShadow, 0.0, 1.0);
	return fShadow;
}














