#define MODE_BUMP		1
#define MODE_PARALLAX	2
#define MODE_RELIEF		3

void computeLightVectorsBump(in vec3 t, in vec3 b, in vec3 n){
	vec3 tmpVec; 

	for(int i = 0; i < MAX_LIGHT_COUNT; i++){
		if(light_count == i) break;
		if(gl_LightSource[i].position.w == 0.0){
			tmpVec = -gl_LightSource[i].position.xyz;					
		}else{
			tmpVec = gl_LightSource[i].position.xyz - _vertexMV.xyz;	
		}

		vPixToLightMV[i] = tmpVec;
	
		vPixToLightTBN[i].x = dot(tmpVec, t);
		vPixToLightTBN[i].y = dot(tmpVec, b);
		vPixToLightTBN[i].z = dot(tmpVec, n);
		//View vector
		tmpVec = -_vertexMV.xyz;
		vPixToEyeTBN[i].x = dot(tmpVec, t);
		vPixToEyeTBN[i].y = dot(tmpVec, b);
		vPixToEyeTBN[i].z = dot(tmpVec, n);
	}
}