uniform float dvd_lightBleedBias = 0.001;
uniform float worldHalfExtent;

float linearStep(in float min, in float max, in float v){
  return clamp((v-min)/(max-min),0.0,1.0);
}  

float ReduceLightBleeding(in float p_max)  {  
   return linearStep(dvd_lightBleedBias, 1, p_max);  
}  
/*
float chebyshevUpperBound(float distance, in vec2 posInDM, in int layer){

    // We retrive the two moments previously stored (depth and depth*depth)
    vec2 moments = texture(texDepthMapFromLightArray, vec3(posInDM, layer)).rg;

    // Surface is fully lit. as the current fragment is before the light occluder
    if (distance <= moments.x)  return 1.0;

    // The fragment is either in shadow or penumbra. 
    //We now use chebyshev's upperBound to check how likely this pixel is to be lit
    float variance = moments.y - (moments.x * moments.x);
    variance = max(variance, 0.0002);
    
    float d = distance - moments.x;
    return (variance / (variance + d*d));
}*/

void applyShadowDirectional(in float NdotL, in int shadowIndex, inout float shadow) {
    ///Limit maximum number of lights that cast shadows
    if(!dvd_enableShadowMapping) 
        return;
            
    vec4 shadow_coord = _shadowCoord[shadowIndex];
    float tOrtho[] = { worldHalfExtent / 11.0, worldHalfExtent / 5.0, worldHalfExtent };



    int i = 0;   
    vec3 vPosInDM;
    for(; i < 3; ++i){
        //Map i
        vPosInDM = vec3(shadow_coord.xy/tOrtho[i], shadow_coord.z) / (shadow_coord.w);
        vPosInDM = (vPosInDM + 1.0) * 0.5;
        if(vPosInDM.x >= 0.0 && vPosInDM.y >= 0.0 && vPosInDM.x <= 1.0 && vPosInDM.y <= 1.0){
            //shadow = ReduceLightBleeding(chebyshevUpperBound(shadow_coord.w, vPosInDM.xy, i));
            shadow = texture(texDepthMapFromLightArray, vec4(vPosInDM.xy, i, vPosInDM.z));
            return;
        }
    }
}