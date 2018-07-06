uniform bool  enableFog;
uniform float fogStart;
uniform float fogEnd;
uniform float fogDensitySqrtNeg;
uniform vec3  fogColor;

const float LOG2 = 1.442695;
float zDepth = gl_FragCoord.z / gl_FragCoord.w;

void applyFog(inout vec4 color){
	if(enableFog){
		
		float fogFactor = exp2( -gl_Fog.density * 
				   			     gl_Fog.density * 
								 zDepth * 
								 zDepth * 
								 LOG2 );
		fogFactor = clamp(fogFactor, 0.0, 1.0);
		color = mix(gl_Fog.color, color, fogFactor );
	}
}