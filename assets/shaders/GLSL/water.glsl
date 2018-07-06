-- Vertex
#include "vboInputData.vert"
varying vec3 vPixToLight;		
varying vec3 vPixToEye;	
varying vec4 vPosition;
varying vec4 vVertexFromLightView;
varying vec4 texCoord[2];

uniform vec3 water_bb_min;
uniform vec3 water_bb_max;
uniform mat4 lightProjectionMatrix;

void main(void)
{
	computeData();
	
	
	vPosition = vertexData;
	vec3 vPositionNormalized = (vertexData.xyz - water_bb_min.xyz) / (water_bb_max.xyz - water_bb_min.xyz);
	texCoord[0].st = vPositionNormalized.xz;
	
	vPixToLight = -(gl_LightSource[0].position.xyz);
	vPixToEye = -vec3(gl_ModelViewMatrix * vertexData);	

	vVertexFromLightView = lightProjectionMatrix * vertexData;
	gl_Position = gl_ModelViewProjectionMatrix * vertexData;
	
}

-- Fragment

uniform float screenWidth;
uniform float screenHeight;
uniform float noise_tile;
uniform float noise_factor;
uniform float time;
uniform float water_shininess;

uniform sampler2D texWaterReflection;
uniform sampler2D texWaterNoiseNM;

varying vec3 vPixToLight;
varying vec3 vPixToEye;	
varying vec4 vPosition;
varying vec4 vVertexFromLightView;
varying vec4 texCoord[2];

// SHADOW MAPPING //
uniform int depthMapCount;
uniform sampler2DShadow texDepthMapFromLight0;
uniform sampler2DShadow texDepthMapFromLight1;
uniform sampler2DShadow texDepthMapFromLight2;
#define Z_TEST_SIGMA 0.0001
////////////////////

float ShadowMapping();

float Fresnel(vec3 incident, vec3 normal, float bias, float power);

void main (void)
{
	vec2 uvNormal0 = texCoord[0].st*noise_tile;
	uvNormal0.s += time*0.01;
	uvNormal0.t += time*0.01;
	vec2 uvNormal1 = texCoord[0].st*noise_tile;
	uvNormal1.s -= time*0.01;
	uvNormal1.t += time*0.01;
		
	vec3 normal0 = texture2D(texWaterNoiseNM, uvNormal0).rgb * 2.0 - 1.0;
	vec3 normal1 = texture2D(texWaterNoiseNM, uvNormal1).rgb * 2.0 - 1.0;
	vec3 normal = normalize(normal0+normal1);
	
	vec2 uvReflection = vec2(gl_FragCoord.x/screenWidth, gl_FragCoord.y/screenHeight);
	
	vec2 uvFinal = uvReflection.xy + noise_factor*normal.xy;
	gl_FragColor = texture2D(texWaterReflection, uvFinal);
	
	vec3 N = normalize(vec3(gl_ModelViewMatrix * vec4(normal.x, normal.z, normal.y, 0.0)));
	vec3 L = normalize(vPixToLight);
	vec3 V = normalize(vPixToEye);
	float iSpecular = pow(clamp(dot(reflect(-L, N), V), 0.0, 1.0), water_shininess);
	
	gl_FragColor += gl_LightSource[0].specular * iSpecular;
	gl_FragColor = clamp(gl_FragColor, vec4(0.0, 0.0, 0.0, 0.0),  vec4(1.0, 1.0, 1.0, 1.0));
	
	/////////////////////////
	// SHADOW MAPS
	float distance_max = 200.0;
	float shadow = 1.0;
	float distance = length(vPixToEye);
	if(distance < distance_max) {
		if(depthMapCount > 0){
			shadow = ShadowMapping();
			shadow = shadow * 0.5 + 0.5;
		}

	}
	/////////////////////////

	gl_FragColor *= shadow;

	// FRESNEL ALPHA
	gl_FragColor.a	= Fresnel(V, N, 0.5, 2.0);

}


float Fresnel(vec3 incident, vec3 normal, float bias, float power)
{
	float scale = 1.0 - bias;
	return bias + pow(1.0 - dot(incident, normal), power) * scale;
}


float ShadowMapping()
{
	float fShadow = 0.0;
						
	float tOrtho[3];
	tOrtho[0] = 5.0;
	tOrtho[1] = 10.0;
	tOrtho[2] = 50.0;

	bool ok = false;
	int id = 0;
	vec3 vPixPosInDepthMap;

	for(int i = 0; i < 3; i++){

		if(!ok){

			vPixPosInDepthMap = vec3(vVertexFromLightView.xy/tOrtho[i], vVertexFromLightView.z) / (vVertexFromLightView.w);
			vPixPosInDepthMap = (vPixPosInDepthMap + 1.0) * 0.5;					// de l'intervale [-1 1] à [0 1]
				
			if(vPixPosInDepthMap.x >= 0.0 && vPixPosInDepthMap.y >= 0.0 && vPixPosInDepthMap.x <= 1.0 && vPixPosInDepthMap.y <= 1.0){
				id = i;
				ok = true;
			}
		}
	}
		
	if(ok){

		vec4 vDepthMapColor = vec4(0.0, 0.0, 0.0, 1.0);
		switch(id){
			case 0:
				vDepthMapColor = shadow2D(texDepthMapFromLight0, vPixPosInDepthMap);
				break;
			case 1:
				vDepthMapColor = shadow2D(texDepthMapFromLight1, vPixPosInDepthMap);
				break;
			case 2:
				vDepthMapColor = shadow2D(texDepthMapFromLight2, vPixPosInDepthMap);
				break;
		};

		if((vDepthMapColor.z+Z_TEST_SIGMA) < vPixPosInDepthMap.z){
			fShadow = clamp((vPixPosInDepthMap.z - vDepthMapColor.z)*10.0, 0.0, 1.0);
		}else{
			fShadow = 1.0;
		}

		fShadow = clamp(fShadow, 0.0, 1.0);
	}else{
		fShadow = 1.0;
	}
	
	return fShadow;
}
