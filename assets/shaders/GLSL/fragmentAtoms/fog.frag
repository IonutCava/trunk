uniform bool  enableFog = true;
uniform float fogStart;
uniform float fogEnd;
uniform float fogDensity;
uniform int   fogMode; //<only exp2 for now
uniform int   fogDetailLevel;
uniform vec3  fogColor;

const float LOG2 = 1.442695;
float zDepth = gl_FragCoord.z / gl_FragCoord.w;

void applyFog(inout vec4 color){
	if(!enableFog) return;

	float fogFactor = exp2( -fogDensity * fogDensity * zDepth * zDepth * LOG2 );
	fogFactor = clamp(fogFactor, 0.0, 1.0);
	color.rgb = mix(fogColor, color.rgb, fogFactor);

}  