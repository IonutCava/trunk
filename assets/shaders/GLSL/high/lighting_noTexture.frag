//based on: http://yannick.gerometta.free.fr/base.php?id=glsldemo
varying vec4 vPixToLightTBN[1];	// Pixel to Light Vector
varying vec3 vPixToEyeTBN;		// Pixel to Eye Vector
varying vec3 vVertexMV;
varying vec3 vPixToLightMV;
varying vec3 vLightDirMV;
varying vec3 vNormalMV;
varying vec4 texCoord[2];
varying vec3 lightDir,halfVector;
//Material properties
//vec4 ambient = material[0];
//vec4 diffuse = material[1];
//vec4 specular = material[2];
//float shininess = material[3].x;
//vec4 emmissive = vec4(material[3].yzw,1.0f);
uniform mat4 material;

uniform float parallax_factor;
uniform float relief_factor;
uniform float tile_factor;
uniform bool enableFog;

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
vec4  Phong(vec3 vNormalTBN, vec3 vEyeTBN, vec4 vLightTBN);

const float LOG2 = 1.442695;

void main (void){

	gl_FragDepth = gl_FragCoord.z;
	vec2 uv = texCoord[0].st*tile_factor;
	vec4 color = Phong(vNormalMV, vPixToEyeTBN, vPixToLightTBN[0]);
	if(color.a < 0.2) discard;	

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
	gl_FragData[0] = color;
}

vec4 Phong(vec3 vNormalTBN, vec3 vEyeTBN, vec4 vLightTBN){

	float att = 1.0;
	//If the light isn't directional, compute attenuation
	if(vLightTBN.w != LIGHT_DIRECTIONAL) {
		float dist = length(vLightTBN.xyz);
		att = 1.0/(gl_LightSource[0].constantAttenuation + gl_LightSource[0].linearAttenuation*dist + gl_LightSource[0].quadraticAttenuation*dist*dist);
		att = max(att, 0.0);	
	}
	//Normalized Light/Normal/Eye vectors in TBN space
	vec3 L = normalize(vLightTBN.xyz);
	vec3 N = normalize(vNormalTBN.xyz);
	vec3 V = normalize(vEyeTBN.xyz);
	//vec4 ambient = material[0];
	//vec4 diffuse = material[1];
	//vec4 specular = material[2];
	//float shininess = material[3].x;
	//vec4 emmissive = vec4(material[3].yzw,1.0f);
	
	
	//Diffuse intensity
	float iDiffuse = max(dot(L, N), 0.0);
	//Specular intensity based on material shininess
	float iSpecular = pow(max(dot(reflect(-L, N), V), 0.0), material[3].x );
	//Ambient color	
	vec4 cAmbient = gl_LightSource[0].ambient * material[0];
	//Diffuse color
	vec4 cDiffuse = gl_LightSource[0].diffuse * material[1] * iDiffuse;	
	//Specular color
	vec4 cSpecular = gl_LightSource[0].specular * material[2] * iSpecular;	

	//If we have a spot light
	if(vLightTBN.w > 1.5){
		//Handle the spot's halo
		if(dot(normalize(vPixToLightMV.xyz), normalize(-vLightDirMV.xyz)) < gl_LightSource[0].spotCosCutoff){
			cDiffuse = vec4(0.0, 0.0, 0.0, 1.0);
			cSpecular = vec4(0.0, 0.0, 0.0, 1.0);
		}else{
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
		}
	//If it's not a spot light
	}else{
		//ToDo
	}
	cAmbient = cAmbient + gl_FrontLightModelProduct.sceneColor * material[0];

	return  cAmbient  + (cDiffuse + cSpecular) * att;	
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
		
			vPixPosInDepthMap = vec3(texCoord[1].xy/tOrtho[i], texCoord[1].z) / (gl_TexCoord[1].w);
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