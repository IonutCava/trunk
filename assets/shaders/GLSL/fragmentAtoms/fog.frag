
const float LOG2 = 1.442695;

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