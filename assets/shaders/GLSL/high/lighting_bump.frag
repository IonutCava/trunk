//based on: http://yannick.gerometta.free.fr/base.php?id=glsldemo
varying vec4 vPixToLightTBN[1];	// Pixel to Light Vector
varying vec3 vPixToEyeTBN;		// Pixel to Eye Vector
varying vec3 vVertexMV;
varying vec3 vPixToLightMV;
varying vec3 vLightDirMV;
varying vec3 vNormalMV;
varying vec4 texCoord[2];

//Textures
//0 -> no texture
//1 -> use texDiffuse0
//2 -> add texDiffuse0 + texDiffuse1
uniform int textureCount;
//true -> use opacity map
uniform bool hasOpacity;
//true -> use specular map 
uniform bool hasSpecular;
uniform sampler2D texDiffuse0;
uniform sampler2D texDiffuse1;
//Normal or BumpMap
uniform sampler2D texBump;
//Opacity and specular maps
uniform sampler2D opacityMap;
uniform sampler2D specularMap;
//Material properties
//vec4 ambient = material[0];
//vec4 diffuse = material[1];
//vec4 specular = material[2];
//float shininess = material[3].x;
//vec4 emmissive = vec4(material[3].yzw,1.0f);
uniform mat4 material;

#define MODE_PHONG		0
#define MODE_BUMP		1
#define MODE_PARALLAX	2
#define MODE_RELIEF		3

uniform int mode;
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
vec4  Phong(vec2 uv, vec3 vNormalTBN, vec3 vEyeTBN, vec4 vLightTBN);
vec4  NormalMapping(vec2 uv, vec3 vPixToEyeTBN, vec4 vPixToLightTBN, bool bParallax);
float ReliefMapping_RayIntersection(in vec2 A, in vec2 AB);
vec4  ReliefMapping(vec2 uv);
vec4  applyFog(in vec4 color);

const float LOG2 = 1.442695;

void main (void){

	gl_FragDepth = gl_FragCoord.z;
	vec4 vPixToLightTBNcurrent = vPixToLightTBN[0];
	
	vec4 color;
	//Else, use appropriate lighting / bump mapping / relief mapping / normal mapping
	if(mode == MODE_PHONG)
		color = Phong(texCoord[0].st*tile_factor, vNormalMV, vPixToEyeTBN, vPixToLightTBNcurrent);
	else if(mode == MODE_RELIEF)
		color = ReliefMapping(texCoord[0].st*tile_factor);
	else if(mode == MODE_BUMP)
		color = NormalMapping(texCoord[0].st*tile_factor, vPixToEyeTBN, vPixToLightTBNcurrent, false);
	else if(mode == MODE_PARALLAX)
		color = NormalMapping(texCoord[0].st*tile_factor, vPixToEyeTBN, vPixToLightTBNcurrent, true);
	
	if(hasOpacity ){
		vec4 alpha = texture2D(opacityMap, texCoord[0].st*tile_factor);
		if(alpha.a < 0.2) discard;
	}else{
		if(color.a < 0.2) discard;	
	}
	if(enableFog){
		color = applyFog(color);
	}
	gl_FragData[0] = color;
}


float ReliefMapping_RayIntersection(in vec2 A, in vec2 AB){

	const int num_steps_lin = 10;
	const int num_steps_bin = 15;
	
	float linear_step = 1.0 / (float(num_steps_lin));
	//Current depth position
	float depth = 0.0; 
	//Best match found (starts with last position 1.0)
	float best_depth = 1.0;
	float step = linear_step;
	//Search from front to back for first point inside the object
	for(int i=0; i<num_steps_lin-1; i++){
		depth += step;
		float h = 1.0 - texture2D(texBump, A+AB*depth).a;
		
		if (depth >= h) {
			best_depth = depth; //Store best depth
			i = num_steps_lin-1;
		}
	}
	//The point of intersection is found between (depth) and (depth-step)
	//so start from (depth - step/2)
	step = linear_step/2.0;
	depth = best_depth - step;
	// binary search
	for(int i=0; i<num_steps_bin; i++){

		float h = 1.0 - texture2D(texBump, A+AB*depth).a;
		
		step /= 2.0;
		if (depth >= h) {
			best_depth = depth;
			depth -= step;
		}
		else {
			best_depth = depth;
			depth += step;
		}
	}
	return best_depth;
}


vec4 ReliefMapping(vec2 uv){

	vec4 vPixToLightTBNcurrent = vPixToLightTBN[0];
	vec3 viewVecTBN = normalize(vPixToEyeTBN);
	//Size and search starting position in texture space
	vec2 A = uv;
	vec2 AB = relief_factor * vec2(-viewVecTBN.x, viewVecTBN.y)/viewVecTBN.z;

	float h = ReliefMapping_RayIntersection(A, AB);
	
	vec2 uv_offset = h * AB;
	
	vec3 p = vVertexMV;
	vec3 v = normalize(p);
	//Compute light direction
	p += v*h*viewVecTBN.z;	
	
	float near = 0.1;
	float far = 800.0;	
	vec2 planes;
	planes.x = -far/(far-near);
	planes.y = -far*near/(far-near);
	gl_FragDepth =((planes.x*p.z+planes.y)/-p.z);
	
	return NormalMapping(uv+uv_offset, vPixToEyeTBN, vPixToLightTBNcurrent, false);
}

vec4 NormalMapping(vec2 uv, vec3 vPixToEyeTBN, vec4 vPixToLightTBN, bool bParallax){

	vec3 lightVecTBN = normalize(vPixToLightTBN.xyz);
	vec3 viewVecTBN = normalize(vPixToEyeTBN);

	vec2 vTexCoord = uv;
	if(bParallax) {			
		//Offset, scale and biais
		float height = texture2D(texBump, uv).a;
		vTexCoord = uv + ((height-0.5)* parallax_factor * (vec2(viewVecTBN.x, -viewVecTBN.y)/viewVecTBN.z));
	}
	//Normal mapping in TBN space
	vec3 normalTBN = normalize(texture2D(texBump, vTexCoord).xyz * 2.0 - 1.0);
	//Lighting
	return Phong(vTexCoord, normalTBN, vPixToEyeTBN, vPixToLightTBN);
}

vec4 Phong(vec2 uv, vec3 vNormalTBN, vec3 vEyeTBN, vec4 vLightTBN){

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
	
	//Use material specular value
	vec4 specular = material[2];
	//If we have a specular map
	if(hasSpecular){
		//use the specular map instead
		specular = texture2D(specularMap,uv);
	}
	//Diffuse intensity
	float iDiffuse = max(dot(L, N), 0.0);
	//Specular intensity based on material shininess
	float iSpecular = pow(clamp(dot(reflect(-L, N), V), 0.0, 1.0), material[3].x );
	//Ambient color	
	vec4 cAmbient = gl_LightSource[0].ambient * material[0];
	//Diffuse color
	vec4 cDiffuse = gl_LightSource[0].diffuse * material[1] * iDiffuse;	
	//Specular color
	vec4 cSpecular = gl_LightSource[0].specular * specular * iSpecular;	
	//Adding diffuse texture(s)
	vec4 tBase[2];
	//set the texture color vector to 1 so we don't change final color if no texture
	tBase[0]  = vec4(1.0,1.0,1.0,1.0);
	//If we have at least 1 texture
	if(textureCount > 0){
		//Get the texture color
		tBase[0] = texture2D(texDiffuse0, uv);
		//If the texture's alpha channel is less than 1/3, discard current fragment
		if(tBase[0].a < 0.3) discard;
		//If we have a second diffuse texture
		if(textureCount > 1){
			//Add the 2 color's togheter
			tBase[1] = texture2D(texDiffuse1, uv);
			tBase[0] = tBase[0] + tBase[1];
		} 
	}
	float shadow = 1.0;
	//If we have a spot light
	if(vLightTBN.w > 1.5){
		//Handle the spot's halo
		if(dot(normalize(vPixToLightMV.xyz), normalize(-vLightDirMV.xyz)) < gl_LightSource[0].spotCosCutoff){
			cDiffuse = vec4(0.0, 0.0, 0.0, 1.0);
			cSpecular = vec4(0.0, 0.0, 0.0, 1.0);
		}else{
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
					tBase[0].xyz = mix(tBase[0].xyz, cProjected.xyz, shadow/2.0);
				}
			}
		}
	//If it's not a spot light
	}else{
		//ToDo
	}
	cAmbient = cAmbient + gl_FrontLightModelProduct.sceneColor * material[0];
	//Add all values togheter to compute the final fragment color
	vec4 color = cAmbient * tBase[0] + (cDiffuse * tBase[0] + cSpecular) * att;

	return color;	
}

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
				//break; // no need to continue
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

vec4 applyFog(in vec4 color){
	float z = gl_FragCoord.z / gl_FragCoord.w;
	float fogFactor = exp2( -gl_Fog.density * 
					   gl_Fog.density * 
					   z * 
					   z * 
					LOG2 );
	fogFactor = clamp(fogFactor, 0.0, 1.0);

	return mix(gl_Fog.color, color, fogFactor );
}