//Normal or BumpMap
uniform sampler2D texBump;
uniform float parallax_factor;
uniform float relief_factor;
uniform float zNear;
uniform float zFar;

#define MODE_PHONG      0
#define MODE_BUMP		1
#define MODE_PARALLAX	2
#define MODE_RELIEF		3

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
	
	vec2 planes;
	planes.x = -zFar/(zFar-zNear);
	planes.y = -zFar*zNear/(zFar-zNear);
	gl_FragDepth =((planes.x*p.z+planes.y)/-p.z);
	
	return NormalMapping(uv+uv_offset, vPixToEyeTBN, vPixToLightTBNcurrent, false);
}


