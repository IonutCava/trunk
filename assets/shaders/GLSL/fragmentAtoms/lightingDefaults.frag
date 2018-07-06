uniform bool  dvd_enableFog = true;
uniform float fogStart;
uniform float fogEnd;
uniform float fogDensity;
uniform int   fogMode; //<only exp2 for now
uniform int   fogDetailLevel;
uniform vec3  fogColor;

const float LOG2 = 1.442695;

#if defined(SKIP_HARDWARE_CLIPPING)
in float dvd_ClipDistance[MAX_CLIP_PLANES];
#else
in float gl_ClipDistance[MAX_CLIP_PLANES];
#endif

void computeData() {

#if defined(SKIP_HARDWARE_CLIPPING)
    if(dvd_ClipDistance[0] < 0) discard;
#else
    if(gl_ClipDistance[0] < 0) discard;
#endif

}

void applyFog(inout vec4 color){
    if(!dvd_enableFog)
        return;

    float zDepth = gl_FragCoord.z / gl_FragCoord.w;
    color.rgb = mix(fogColor, 
                    color.rgb,
                    clamp(exp2( -fogDensity * fogDensity * zDepth * zDepth * LOG2 ), 0.0, 1.0));

}  