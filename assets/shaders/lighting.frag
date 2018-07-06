

varying vec4 vPixToLightTBN[1];	// Vecteur du pixel courant à la lumière
varying vec3 vPixToEyeTBN;		// Vecteur du pixel courant à l'oeil
varying vec3 vVertexMV;
varying vec3 vNormalMV;
varying vec3 vPixToLightMV;
varying vec3 vLightDirMV;

//Textures
uniform int textureCount;
uniform sampler2D texDiffuse0;
uniform sampler2D texDiffuse1;
uniform sampler2D texBump;

//Material properties
uniform mat4 material;


#define MODE_PHONG		0
#define MODE_BUMP		1
#define MODE_PARALLAX	2
#define MODE_RELIEF		3

uniform int mode;
uniform float parallax_factor;
uniform float relief_factor;
uniform float tile_factor;


// SHADOW MAPPING //
uniform int depth_map_size;
uniform int enable_shadow_mapping;	// 0->no  1->shadow mapping  2->shadow mapping + projected texture
uniform sampler2DShadow texDepthMapFromLight;
//uniform sampler2D texDiffuseProjected;
#define Z_TEST_SIGMA 0.0001
////////////////////

#define LIGHT_DIRECTIONAL		0.0
#define LIGHT_OMNIDIRECTIONAL	1.0
#define LIGHT_SPOT				2.0

//float ShadowMapping(vec4 vVertexFromLightView, out vec3 vPixPosInDepthMap);
vec4  Phong(vec2 uv, vec3 vNormalTBN, vec3 vEyeTBN, vec4 vLightTBN);
vec4  NormalMapping(vec2 uv, vec3 vPixToEyeTBN, vec4 vPixToLightTBN, bool bParallax);
float ReliefMapping_RayIntersection(in vec2 A, in vec2 AB);
vec4  ReliefMapping(vec2 uv);

void main (void)
{
	gl_FragDepth = gl_FragCoord.z;
	vec4 vPixToLightTBNcurrent = vPixToLightTBN[0];
	
	vec4 colorOut = vec4(1.0, 0.0, 0.0, 1.0);


	if(mode == MODE_PHONG)
		colorOut = Phong(gl_TexCoord[0].st*tile_factor, vec3(0.0, 0.0, 1.0), vPixToEyeTBN, vPixToLightTBNcurrent);

	else if(mode == MODE_RELIEF)
		colorOut = ReliefMapping(gl_TexCoord[0].st*tile_factor);
		
	else if(mode == MODE_BUMP)
		colorOut = NormalMapping(gl_TexCoord[0].st*tile_factor, vPixToEyeTBN, vPixToLightTBNcurrent, false);
	
	else if(mode == MODE_PARALLAX)
		colorOut = NormalMapping(gl_TexCoord[0].st*tile_factor, vPixToEyeTBN, vPixToLightTBNcurrent, true);


	gl_FragColor = colorOut;

}


float ReliefMapping_RayIntersection(in vec2 A, in vec2 AB)
{
	const int num_steps_lin = 10;
	const int num_steps_bin = 15;
	
	float linear_step = 1.0 / (float(num_steps_lin));
	float depth = 0.0; // current depth position
	
	// best match found (starts with last position 1.0)
	float best_depth = 1.0;
	float step = linear_step;
	
	// search from front to back for first point inside the object
	for(int i=0; i<num_steps_lin-1; i++){
		depth += step;
		float h = 1.0 - texture2D(texBump, A+AB*depth).a;
		
		if (depth >= h) { // h est dans la heightmap
			best_depth = depth; // store best depth
			i = num_steps_lin-1;
		}
		
	}
	
	
	// l'intersection se situe entre (depth) et (depth-step);
	// on se place donc à (depth - step/2) pour commencer
	step = linear_step/2.0;
	depth = best_depth - step;
	
	// recherche par dichotomie
	for(int i=0; i<num_steps_bin; i++)
	{
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


vec4 ReliefMapping(vec2 uv)
{	
	vec4 vPixToLightTBNcurrent = vPixToLightTBN[0];
	vec3 viewVecTBN = normalize(vPixToEyeTBN);
	
	// size and start position of search in texture space
	vec2 A = uv;
	vec2 AB = relief_factor * vec2(-viewVecTBN.x, viewVecTBN.y)/viewVecTBN.z;

	float h = ReliefMapping_RayIntersection(A, AB);
	
	vec2 uv_offset = h * AB;
	

	vec3 p = vVertexMV;
	vec3 v = normalize(p);
		
	// compute light direction
	p += v*h*viewVecTBN.z;	
	
	
	float near = 0.1;
	float far = 800.0;	
	vec2 planes;
	planes.x = -far/(far-near);
	planes.y = -far*near/(far-near);
	gl_FragDepth =((planes.x*p.z+planes.y)/-p.z);
	


	return NormalMapping(uv+uv_offset, vPixToEyeTBN, vPixToLightTBNcurrent, false);
}



vec4 NormalMapping(vec2 uv, vec3 vPixToEyeTBN, vec4 vPixToLightTBN, bool bParallax)
{	
	vec3 lightVecTBN = normalize(vPixToLightTBN.xyz);
	vec3 viewVecTBN = normalize(vPixToEyeTBN);

	vec2 vTexCoord = uv;
	if(bParallax) {			
		// Calculate offset, scale & biais
		float height = texture2D(texBump, uv).a;
		vTexCoord = uv + ((height-0.5)* parallax_factor * (vec2(viewVecTBN.x, -viewVecTBN.y)/viewVecTBN.z));
	}
	
	// on trouve la normale pertubée dans l'espace TBN
	vec3 normalTBN = normalize(texture2D(texBump, vTexCoord).xyz * 2.0 - 1.0);
	
//// ECLAIRAGE :
	return Phong(vTexCoord, normalTBN, vPixToEyeTBN, vPixToLightTBN);
}




vec4 Phong(vec2 uv, vec3 vNormalTBN, vec3 vEyeTBN, vec4 vLightTBN)
{

	float att = 1.0;
	if(vLightTBN.w != LIGHT_DIRECTIONAL) {
		float dist = length(vLightTBN.xyz);
		att = 1.0/(gl_LightSource[0].constantAttenuation + gl_LightSource[0].linearAttenuation*dist + gl_LightSource[0].quadraticAttenuation*dist*dist);
		att = max(att, 0.0);	
	}
	
	vec3 L = normalize(vLightTBN.xyz);
	vec3 N = normalize(vNormalTBN.xyz);
	vec3 V = normalize(vEyeTBN.xyz);
	
	//vec4 ambient = material[0];
	//vec4 diffuse = material[1];
	//vec4 specular = material[2];
	//float shininess = material[3].x;
	//vec4 emmissive = vec4(material[3].xyx,1.0f);
	
//// ECLAIRAGE :

	float iDiffuse = max(dot(L, N), 0.0);	// Intensité diffuse
	float iSpecular = pow(clamp(dot(reflect(-L, N), V), 0.0, 1.0), material[3].x );
	
	
	//vec4 cAmbient = gl_LightSource[0].ambient * gl_FrontMaterial.ambient;
	//vec4 cDiffuse = gl_LightSource[0].diffuse * gl_FrontMaterial.diffuse * iDiffuse;	
	//vec4 cSpecular = gl_LightSource[0].specular * gl_FrontMaterial.specular * iSpecular;	
	
	vec4 cAmbient = gl_LightSource[0].ambient * material[0];
	vec4 cDiffuse = gl_LightSource[0].diffuse * material[1] * iDiffuse;	
	vec4 cSpecular = gl_LightSource[0].specular * material[2] * iSpecular;	
	
	if(textureCount > 0){
		vec4 base = texture2D(texDiffuse0, uv);	// Couleur diffuse
		if(textureCount == 2) base += texture2D(texDiffuse1, uv);
		
		cAmbient *= base;
		cDiffuse *= base;
	}


		
	
	// Si c'est une lumière SPOT
	if(vLightTBN.w > 1.5)
	{
		// Gestion du halo du spot
		if(dot(normalize(vPixToLightMV.xyz), normalize(-vLightDirMV.xyz)) < gl_LightSource[0].spotCosCutoff)
		{
			cDiffuse = vec4(0.0, 0.0, 0.0, 1.0);
			cSpecular = vec4(0.0, 0.0, 0.0, 1.0);
		}
		else
		{
			// Shadow mapping :
			//if(enable_shadow_mapping != 0) {
			//	vec3 vPixPosInDepthMap;
			//	float shadow = ShadowMapping(gl_TexCoord[1], vPixPosInDepthMap);
			//	cDiffuse = (shadow) * cDiffuse;
			//	cSpecular = (shadow) * cSpecular;
			//	
			//	// Texture projection :
			//	if(enable_shadow_mapping == 2) {
			//		vec4 cProjected = texture2D(texDiffuseProjected, vec2(vPixPosInDepthMap.s, 1.0-vPixPosInDepthMap.t));
			//		base.xyz = mix(base.xyz, cProjected.xyz, shadow/2.0);
			//	}
			//}
		}
	}
	// Si c'est pas une lumière SPOT
	else
	{

	}
	vec4 colorOut = cAmbient + (cDiffuse + cSpecular) * att;
	
	return colorOut;	
}
/*
float ShadowMapping(vec4 vVertexFromLightView, out vec3 vPixPosInDepthMap)
{
	float fShadow = 0.0;
						
	vec2 tOffset[3*3];
	tOffset[0] = vec2(-1.0, -1.0); tOffset[1] = vec2(0.0, -1.0); tOffset[2] = vec2(1.0, -1.0);
	tOffset[3] = vec2(-1.0,  0.0); tOffset[4] = vec2(0.0,  0.0); tOffset[5] = vec2(1.0,  0.0);
	tOffset[6] = vec2(-1.0,  1.0); tOffset[7] = vec2(0.0,  1.0); tOffset[8] = vec2(1.0,  1.0);


	vPixPosInDepthMap = vVertexFromLightView.xyz/vVertexFromLightView.w;	// homogénisation
	vPixPosInDepthMap = (vPixPosInDepthMap + 1.0) * 0.5;					// de l'intervale [-1 1] à [0 1]
	
	
	vec4 vDepthMapColor; = shadow2D(texDepthMapFromLight, vPixPosInDepthMap);

	if((vDepthMapColor.z+Z_TEST_SIGMA) < vPixPosInDepthMap.z)
	{
		fShadow = 0.0;
		
		// Sof Shadow pour les fragments proches
		if( length(vVertexMV.xyz) < 12.0 )
		{
			for(int i=0; i<9; i++)
			{
				vec2 offset = tOffset[i] / (float(depth_map_size));
				// Couleur du pixel sur la depth map
				vec4 vDepthMapColor = shadow2D(texDepthMapFromLight, vPixPosInDepthMap + vec3(offset.s, offset.t, 0.0));
		
				if((vDepthMapColor.z+Z_TEST_SIGMA) < vPixPosInDepthMap.z) {
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

*/























