//based on: http://yannick.gerometta.free.fr/base.php?id=glsldemo

varying vec4 vPixToLightTBN[1];
varying vec3 vPixToEyeTBN;
varying vec3 vVertexMV;
varying vec3 vNormalMV;
varying vec3 vPixToLightMV;
varying vec3 vLightDirMV;
varying vec4 texCoord[2];
uniform int enable_shadow_mapping;

uniform vec4 boneWeightValue;
uniform vec4 boneWeightIndex;
uniform mat4 boneMatrices[32];

uniform mat4 modelViewInvMatrix;
uniform mat4 lightProjectionMatrix;
uniform mat4 modelViewProjectionMatrix;

#define LIGHT_DIRECTIONAL		0.0
#define LIGHT_OMNIDIRECTIONAL	1.0
#define LIGHT_SPOT				2.0
				
void main(void){

	vec4 position = gl_Vertex;
	vec4 pos1 = boneWeightValue.x * (boneMatrices[int(boneWeightIndex.x)] * position);
	vec4 pos2 = boneWeightValue.y * (boneMatrices[int(boneWeightIndex.y)] * position);
	vec4 pos3 = boneWeightValue.z * (boneMatrices[int(boneWeightIndex.z)] * position);
	vec4 pos4 = boneWeightValue.w * (boneMatrices[int(boneWeightIndex.w)] * position);

	vec4 finalPosition =  pos1 + pos2 + pos3 + pos4;
	//Comment the following line to enable skeletal animation
	finalPosition = position;
	//Compute the final vert position
	gl_Position = gl_ModelViewProjectionMatrix * finalPosition;
	//get texture coordinates
	texCoord[0] = gl_MultiTexCoord0;
	//create TBN vectors from VBO data
	vec3 vTangent = gl_MultiTexCoord1.xyz;
	vec3 n = normalize(gl_NormalMatrix * gl_Normal);
	vec3 t = normalize(gl_NormalMatrix * vTangent);
	vec3 b = cross(n, t);
	//Get the light's position in model's space
	vec4 vLightPosMV = gl_LightSource[0].position;	
	//Get the vertex's position in model's space
	vVertexMV = vec3(gl_ModelViewMatrix * gl_Vertex);	
	if(length(vTangent) > 0){
		vNormalMV = b;
	}else{
		vNormalMV = n;
	}

	vec3 tmpVec;
	
	if(vLightPosMV.w == LIGHT_DIRECTIONAL)
		tmpVec = -vLightPosMV.xyz;	
	else
		tmpVec = vLightPosMV.xyz - vVertexMV.xyz;	

	vPixToLightMV = tmpVec;

	vPixToLightTBN[0].x = dot(tmpVec, t);
	vPixToLightTBN[0].y = dot(tmpVec, b);
	vPixToLightTBN[0].z = dot(tmpVec, n);
	// Point or directional?
	vPixToLightTBN[0].w = vLightPosMV.w;	
		
	//View vector
	tmpVec = -vVertexMV;
	vPixToEyeTBN.x = dot(tmpVec, t);
	vPixToEyeTBN.y = dot(tmpVec, b);
	vPixToEyeTBN.z = dot(tmpVec, n);
	
	
	if(length(gl_LightSource[0].spotDirection) > 0.001)	{
		// Spot light
		vLightDirMV = normalize(gl_LightSource[0].spotDirection);
		vPixToLightTBN[0].w = LIGHT_SPOT;
	}else{
		// Not a spot light
		vLightDirMV = gl_LightSource[0].spotDirection;
	}
	
	if(enable_shadow_mapping != 0) {
		// Transformed position 
		vec4 pos = gl_ModelViewMatrix * gl_Vertex;
		// position multiplied by the inverse of the camera matrix
		pos = modelViewInvMatrix * pos;
		// position multiplied by the light matrix. 
		//The vertex's position from the light's perspective
		texCoord[1] = lightProjectionMatrix * pos;
	}
}
