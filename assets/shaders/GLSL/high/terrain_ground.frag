varying vec4 vPixToLightTBN;	// Vecteur du pixel courant à la lumière
varying vec3 vPixToEyeTBN;		// Vecteur du pixel courant à l'oeil
varying vec3 vPosition;
varying vec3 vVertexMV;
varying vec3 vPositionNormalized;
varying vec4 vVertexFromLightView;

uniform sampler2D texDiffuseMap;
uniform sampler2D texNormalHeightMap;
uniform sampler2D texDiffuse0;
uniform sampler2D texDiffuse1;
uniform sampler2D texDiffuse2;
uniform sampler2D texDiffuse3;
uniform sampler2D texWaterCaustics;

uniform float detail_scale;
uniform float diffuse_scale;

uniform bool water_reflection_rendering;
uniform bool alphaTexture;
uniform float water_height;
uniform float time;

// Bounding Box du terrain
uniform vec3 bbox_min;

// SHADOW MAPPING //
uniform int enable_shadow_mapping;
uniform sampler2DShadow texDepthMapFromLight0;
uniform sampler2DShadow texDepthMapFromLight1;
uniform sampler2DShadow texDepthMapFromLight2;
#define Z_TEST_SIGMA 0.0001

// Shadow map sample count for PCF
#define PCF_SAMPLE_COUNT_25 25
#define PCF_SAMPLE_COUNT_16 16
#define PCF_SAMPLE_COUNT_9 9
#define PCF_SAMPLE_COUNT_4 4
#define PCF_SAMPLE_COUNT_1 1

// Shadow filter borders
#define SHADOW_BORDER_1 8.0
#define SHADOW_BORDER_2 16.0
#define SHADOW_BORDER_3 32.0
#define SHADOW_BORDER_4 64.0

// The 5x5 PCF filter kernel
uniform vec2 shadowSampleOffsets[PCF_SAMPLE_COUNT_25];
////////////////////


float ShadowMapping();
float filterFinalShadow(sampler2DShadow depthMap,vec3 vPosInDM, int resolution);
vec4 NormalMapping(vec2 uv, vec3 vPixToEyeTBN, vec4 vPixToLightTBN, bool bParallax);
vec4 ReliefMapping(vec2 uv);
vec4 CausticsColor();
bool isUnderWater();

void main (void)
{
	// Discard the fragments that are underwater when drawing in reflection
	if(water_reflection_rendering && isUnderWater()){
		discard;
	}
	
	vec4 vPixToLightTBNcurrent = vPixToLightTBN;
	
	gl_FragColor = NormalMapping(gl_TexCoord[0].st, vPixToEyeTBN, vPixToLightTBNcurrent, false);
	
	if(isUnderWater()) {
		float alpha = (water_height - vPosition.y) / (2*(water_height - bbox_min.y));
		gl_FragColor = (1-alpha) * gl_FragColor + alpha * CausticsColor();
	}
}

bool isUnderWater(){
	return vPosition.y < water_height;
}

vec4 CausticsColor()
{
	vec2 uv0 = gl_TexCoord[0].st*100.0;
	uv0.s -= time*0.1;
	uv0.t += time*0.1;
	vec4 color0 = texture2D(texWaterCaustics, uv0);
	
	vec2 uv1 = gl_TexCoord[0].st*100.0;
	uv1.s += time*0.1;
	uv1.t += time*0.1;	
	vec4 color1 = texture2D(texWaterCaustics, uv1);
	
	return (color0 + color1) /2;	
}



vec4 NormalMapping(vec2 uv, vec3 vPixToEyeTBN, vec4 vPixToLightTBN, bool bParallax)
{	
	vec3 lightVecTBN = normalize(vPixToLightTBN.xyz);
	vec3 viewVecTBN = normalize(vPixToEyeTBN);

	vec2 uv_detail = uv * detail_scale;
	vec2 uv_diffuse = uv * diffuse_scale;

	
	vec3 normalTBN = texture2D(texNormalHeightMap, uv_detail).rgb * 2.0 - 1.0;
	normalTBN = normalize(normalTBN);
	
	vec4 tBase[3];
	vec4 alpha;
	tBase[0] = texture2D(texDiffuse0, uv_diffuse);
	tBase[1] = texture2D(texDiffuse1, uv_diffuse);	
	tBase[2] = texture2D(texDiffuse2, uv_diffuse);
	if(alphaTexture){
		alpha = texture2D(texDiffuse3, uv_diffuse);
	}
	vec4 DiffuseMap = texture2D(texDiffuseMap, uv);
	
	vec4 cBase;

	if(vPosition.y < water_height)
		cBase = tBase[0];
	else {
		if(alphaTexture){
			cBase = mix(mix(mix(tBase[1], tBase[0], DiffuseMap.r), tBase[2], DiffuseMap.g), alpha, DiffuseMap.a);
		}else{
			cBase = mix(mix(tBase[1], tBase[0], DiffuseMap.r), tBase[2], DiffuseMap.g);			
		}
	}


	float iDiffuse = max(dot(lightVecTBN.xyz, normalTBN), 0.0);	// Intensité diffuse
	float iSpecular = 0;
	
	if(isUnderWater())
		iSpecular = pow(clamp(dot(reflect(-lightVecTBN.xyz, normalTBN), viewVecTBN), 0.0, 1.0), gl_FrontMaterial.shininess )/2.0;
	
	
	/////////////////////////
	// SHADOW MAPS
	float distance_max = 200.0;
	float shadow = 1.0;
	float distance = length(vPixToEyeTBN);
	if(distance < distance_max) {
		if(enable_shadow_mapping != 0){
			shadow = ShadowMapping();
			shadow = 1.0 - (1.0-shadow) * (distance_max-distance) / distance_max;
		}

	}
	/////////////////////////
	
	
	vec4 cAmbient = gl_LightSource[0].ambient * gl_FrontMaterial.ambient;
	vec4 cDiffuse = gl_LightSource[0].diffuse * gl_FrontMaterial.diffuse * iDiffuse * shadow;	
	vec4 cSpecular = gl_LightSource[0].specular * gl_FrontMaterial.specular * iSpecular * shadow;	

	return cAmbient * cBase + cDiffuse * cBase + cSpecular;
}


float ShadowMapping(){

	float fShadow = 0.0;
						
	float tOrtho[3];
	tOrtho[0] = 20.0;
	tOrtho[1] = 100.0;
	tOrtho[2] = 200.0;

	bool ok = false;
	int id = 0;
	vec3 vPixPosInDepthMap;
	for(int i = 0; i < 3; i++){

		if(!ok){
		
			vPixPosInDepthMap = vec3(vVertexFromLightView.xy/tOrtho[i], vVertexFromLightView.z) / (vVertexFromLightView.w);
			vPixPosInDepthMap = (vPixPosInDepthMap + 1.0) * 0.5;

			if(vPixPosInDepthMap.x >= 0.0 && vPixPosInDepthMap.y >= 0.0 && vPixPosInDepthMap.x <= 1.0 && vPixPosInDepthMap.y <= 1.0){
				id = i;
				ok = true;
				break; // no need to continue
			}
		}
	}
	
	if(ok){
		switch(id){
			case 0:
				fShadow = filterFinalShadow(texDepthMapFromLight0,vPixPosInDepthMap, 2048);
				break;
			case 1:
				fShadow = filterFinalShadow(texDepthMapFromLight1,vPixPosInDepthMap, 1024);
				break;
			case 2:
				fShadow = filterFinalShadow(texDepthMapFromLight2,vPixPosInDepthMap, 512);
				break;
		};
	}

	return fShadow;
}


float filterFinalShadow(sampler2DShadow depthMap,vec3 vPosInDM, int resolution){
	float fShadow = 0.0;
						
	vec2 tOffset[3*3];
	tOffset[0] = vec2(-1.0, -1.0); tOffset[1] = vec2(0.0, -1.0); tOffset[2] = vec2(1.0, -1.0);
	tOffset[3] = vec2(-1.0,  0.0); tOffset[4] = vec2(0.0,  0.0); tOffset[5] = vec2(1.0,  0.0);
	tOffset[6] = vec2(-1.0,  1.0); tOffset[7] = vec2(0.0,  1.0); tOffset[8] = vec2(1.0,  1.0);

	vec4 vDepthMapColor = shadow2D(depthMap, vPosInDM);

	if((vDepthMapColor.z+Z_TEST_SIGMA) < vPosInDM.z)
	{
		fShadow = 0.0;
		
		// Sof Shadow pour les fragments proches
		if( length(vVertexMV.xyz) < 12.0 )
		{
			for(int i=0; i<9; i++)
			{
				vec2 offset = tOffset[i] / (float(resolution));
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





