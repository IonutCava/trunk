//based on: http://yannick.gerometta.free.fr/base.php?id=glsldemo
varying vec4 vPixToLightTBN[1];	// Pixel to Light Vector
varying vec3 vPixToEyeTBN;		// Pixel to Eye Vector
varying vec3 vPosition;
varying vec3 vVertexMV;
varying vec3 vPositionNormalized;
varying vec3 vPixToLightMV;
varying vec3 vLightDirMV;
varying vec3 vNormalMV;
varying vec4 texCoord[2];

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
uniform bool enableFog;

// Bounding Box du terrain
uniform vec3 bbox_min;

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

#define LIGHT_DIRECTIONAL		0.0
#define LIGHT_OMNIDIRECTIONAL	1.0
#define LIGHT_SPOT				2.0

float ShadowMapping(out vec3 vPixPosInDepthMap);
float filterFinalShadow(sampler2DShadow depthMap,vec3 vPosInDM, float resolution);
vec4 NormalMapping(vec2 uv, vec3 vPixToEyeTBN, vec4 vPixToLightTBN, bool bParallax);
vec4 ReliefMapping(vec2 uv);
vec4 CausticsColor();
bool isUnderWater();

const float LOG2 = 1.442695;

void main (void)
{
	// Discard the fragments that are underwater when drawing in reflection
	if(water_reflection_rendering && isUnderWater()){
		discard;
	}
	
	vec4 vPixToLightTBNcurrent = vPixToLightTBN[0];
	
	vec4 color = NormalMapping(texCoord[0].st, vPixToEyeTBN, vPixToLightTBNcurrent, false);
	if(enableFog){
		float z = gl_FragCoord.z / gl_FragCoord.w;
		float fogFactor = exp2( -gl_Fog.density * 
				   gl_Fog.density * 
				   z * 
				   z * 
				LOG2 );
		fogFactor = clamp(fogFactor, 0.0, 1.0);
		color = mix(gl_Fog.color, color, fogFactor );
	}

	if(isUnderWater()) {
		float alpha = (water_height - vPosition.y) / (2*(water_height - bbox_min.y));
		color = (1-alpha) * color + alpha * CausticsColor();
	}
	gl_FragColor = color;
}

bool isUnderWater(){
	return vPosition.y < water_height;
}

vec4 CausticsColor()
{
	vec2 uv0 = texCoord[0].st*100.0;
	uv0.s -= time*0.1;
	uv0.t += time*0.1;
	vec4 color0 = texture2D(texWaterCaustics, uv0);
	
	vec2 uv1 = texCoord[0].st*100.0;
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
	
	vec4 cAmbient = gl_LightSource[0].ambient * gl_FrontMaterial.ambient;
	vec4 cDiffuse = gl_LightSource[0].diffuse * gl_FrontMaterial.diffuse * iDiffuse;	
	vec4 cSpecular = gl_LightSource[0].specular * gl_FrontMaterial.specular * iSpecular;
	
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
		// Texture projection :
		if(enable_shadow_mapping == 2) {
			vec4 cProjected = texture2D(texDiffuseProjected, vec2(vPixPosInDepthMap.s, 1.0-vPixPosInDepthMap.t));
			cDiffuse.xyz = mix(cDiffuse.xyz, cProjected.xyz, shadow/2.0);
		}
	}
	

	return cAmbient * cBase + cDiffuse * cBase + cSpecular;
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