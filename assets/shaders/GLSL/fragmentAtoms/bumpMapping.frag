#ifndef _BUMP_MAPPING_FRAG_
#define _BUMP_MAPPING_FRAG_

uniform int bumpMapLightId = 0;

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
        if (depth >= 1.0 - texture(texNormalMap, A+AB*depth).a) {
            best_depth = depth; //Store best depth
            i = num_steps_lin-1;
        }
    }
    //The point of intersection is found between (depth) and (depth-step)
    //so start from (depth - step/2)
    step = linear_step * 0.5;
    depth = best_depth - step;
    // binary search
    for(int i = 0; i < num_steps_bin; ++i){
        step *= 0.5;
        best_depth = depth;
        if (depth >= 1.0 - texture(texNormalMap, A + AB * depth).a) {
            depth -= step;
        }else {
            depth += step;
        }
    }
    return best_depth;
}

vec4 ParallaxMapping(in uint bumpMapLightId, in vec2 uv){
    vec3 lightVecTBN = vec3(0.0);
    switch (dvd_LightSource[bumpMapLightId]._options.x){
        case LIGHT_DIRECTIONAL      : lightVecTBN = -normalize(dvd_LightSource[bumpMapLightId]._position.xyz); break;
        case LIGHT_OMNIDIRECTIONAL  : 
        case LIGHT_SPOT             : lightVecTBN = normalize(_viewDirection + dvd_LightSource[bumpMapLightId]._position.xyz); break;
    };

    vec3 viewVecTBN = normalize(_viewDirection);
    
    //Offset, scale and bias
    vec2 vTexCoord = uv + ((texture(texNormalMap, uv).a - 0.5) * dvd_parallaxFactor * (vec2(viewVecTBN.x, -viewVecTBN.y) / viewVecTBN.z));

    //Normal mapping in TBN space
    return getPixelColor(vTexCoord, normalize(2.0 * texture(texNormalMap, vTexCoord).xyz - 1.0));
}

vec4 ReliefMapping(in int _light, in vec2 uv){
    vec3 viewVecTBN = normalize(_viewDirection);
    //Size and search starting position in texture space
    vec2 AB = dvd_reliefFactor * vec2(-viewVecTBN.x, viewVecTBN.y)/viewVecTBN.z;

    float h = ReliefMapping_RayIntersection(uv, AB);
    
    vec2 uv_offset = h * AB;
    
    vec3 p = _vertexWV.xyz;
    vec3 v = normalize(p);
    //Compute light direction
    p += v*h*viewVecTBN.z;	
    
    vec2 planes;
    planes.x = -dvd_ZPlanesCombined.y / (dvd_ZPlanesCombined.y - dvd_ZPlanesCombined.x);
    planes.y = -dvd_ZPlanesCombined.y * dvd_ZPlanesCombined.x / (dvd_ZPlanesCombined.y - dvd_ZPlanesCombined.x);

    gl_FragDepth =((planes.x * p.z + planes.y) / -p.z);
    
    return getPixelColor(uv + uv_offset, normalize(2.0 * texture(texNormalMap, uv + uv_offset).xyz - 1.0));
}

#endif //_BUMP_MAPPING_FRAG_