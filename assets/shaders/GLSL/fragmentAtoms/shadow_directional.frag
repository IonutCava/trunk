uniform float dvd_lightBleedBias = 0.001;
uniform float worldHalfExtent;

float linearStep(in float min, in float max, in float v){
  return clamp((v-min)/(max-min),0.0,1.0);
}  

float ReduceLightBleeding(in float p_max)  {  
   return linearStep(dvd_lightBleedBias, 1, p_max);  
}  

float chebyshevUpperBound(in vec3 posInDM, in int layer){

    // We retrive the two moments previously stored (depth and depth*depth)
    vec2 moments = texture(texDepthMapFromLightArray, vec3(posInDM.xy, layer)).rg;

    // Surface is fully lit. as the current fragment is before the light occluder
    if (posInDM.z <= moments.x)  return 1.0;

    // The fragment is either in shadow or penumbra. 
    //We now use chebyshev's upperBound to check how likely this pixel is to be lit
    float variance = moments.y - (moments.x * moments.x);
    variance = max(variance, 0.002);
    
    float d = 1.0 - posInDM.z - moments.x;
    return (variance / (variance + d*d));
}

void applyShadowDirectional(in int shadowIndex, inout float shadow) {

    vec4 shadow_coord = _shadowCoord[shadowIndex];
    float tOrtho[3] = float[3]( worldHalfExtent / 11.0, worldHalfExtent / 5.0, worldHalfExtent );

    int i = 0;   
    vec3 vPosInDM;
    for(; i < 3; ++i){
        //Map i  - ortho projection. w-divide not needed
        vPosInDM = vec3(shadow_coord.xy/tOrtho[i], shadow_coord.z);// / (shadow_coord.w); 
        vPosInDM = (vPosInDM + 1.0) * 0.5;
        if(vPosInDM.x >= 0.0 && vPosInDM.y >= 0.0 && vPosInDM.x <= 1.0 && vPosInDM.y <= 1.0){
            shadow = ReduceLightBleeding(chebyshevUpperBound(vPosInDM, i));
            return;
        }
    }
}