uniform float dvd_lightBleedBias = 0.001f;
uniform float worldHalfExtent;

float linearStep(in float min, in float max, in float v){
  return clamp((v-min)/(max-min),0.0,1.0);
}  

void ReduceLightBleeding(inout float p_max)  {  
   p_max = linearStep(dvd_lightBleedBias, 1, p_max);  
}  

float chebyshevUpperBound( float distance){

	// We retrive the two moments previously stored (depth and depth*depth)
	//vec2 moments = texture(texDepthMapFromLightArray,posInDM.xyz).xy;
	vec2 moments;
		
	// Surface is fully lit. as the current fragment is before the light occluder
	if (distance <= moments.x)
		return 1.0 ;
	
	// The fragment is either in shadow or penumbra. We now use chebyshev's upperBound to check
	// How likely this pixel is to be lit (p_max)
	float variance = moments.y - (moments.x*moments.x);
	variance = max(variance,0.00002);
	
	float d = distance - moments.x;
	float p_max = variance / (variance + d*d);
	ReduceLightBleeding(p_max);
	return p_max;
}
	
float filterShadow(in float NdotL, in sampler2DArrayShadow stex, in vec4 posInDM){
	float ret =  texture(stex, posInDM);
	if ((ret - 1) * ret * NdotL != 0) {
		ret *= 0.25;
		ret += shadow2DArrayOffset(stex, posInDM, ivec2(-1,-1)).r * 0.0625;
		ret += shadow2DArrayOffset(stex, posInDM, ivec2(-1, 0)).r * 0.125;
		ret += shadow2DArrayOffset(stex, posInDM, ivec2(-1, 1)).r * 0.0625;
		ret += shadow2DArrayOffset(stex, posInDM, ivec2( 0,-1)).r * 0.125;
		ret += shadow2DArrayOffset(stex, posInDM, ivec2( 0, 1)).r * 0.125;
		ret += shadow2DArrayOffset(stex, posInDM, ivec2( 1,-1)).r * 0.0625;
		ret += shadow2DArrayOffset(stex, posInDM, ivec2( 1, 0)).r * 0.125;
		ret += shadow2DArrayOffset(stex, posInDM, ivec2( 1, 1)).r * 0.0625;
	}
	return ret;
}

void applyShadowDirectional(in float NdotL, in int shadowIndex, inout float shadow) {
	///Limit maximum number of lights that cast shadows
	if(!dvd_enableShadowMapping) return;
			
	vec4 shadow_coord = _shadowCoord[shadowIndex];
    float tOrtho[3] = float[](worldHalfExtent / 11.0, worldHalfExtent / 5.0, worldHalfExtent);

    // transform this fragment's position from view space to scaled light clip space
	// such that the xy coordinates are in [0;1]
	// note there is no need to divide by w for othogonal light sources
   
    for(int i = 0; i < 3; i++){
        //Map i
	    vec3 vPixPosInDepthMap = vec3(shadow_coord.xy/tOrtho[i], shadow_coord.z) / shadow_coord.w;
	    vPixPosInDepthMap = (vPixPosInDepthMap + 1.0) * 0.5;

	    if(vPixPosInDepthMap.x >= 0.0 && vPixPosInDepthMap.y >= 0.0 && vPixPosInDepthMap.x <= 1.0 && vPixPosInDepthMap.y <= 1.0){
		    //shadow = chebyshevUpperBound(shadow_coord.w);
	        shadow = filterShadow(NdotL, texDepthMapFromLightArray, vec4(vPixPosInDepthMap.xy,i,vPixPosInDepthMap.z));
            return;
        }
    }
}