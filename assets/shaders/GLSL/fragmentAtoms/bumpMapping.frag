//Normal or BumpMap
layout(binding = TEXTURE_NORMALMAP) uniform sampler2D texNormalMap;

uniform float parallax_factor = 1.0f;
uniform float relief_factor = 1.0f;
uniform int bumpMapLightId = 0;

const int num_steps_lin = 10;
const int num_steps_bin = 15;
float linear_step = 1.0 / (float(num_steps_lin));

#if defined(USE_RELIEF_MAPPING)
float ReliefMapping_RayIntersection(in vec2 A, in vec2 AB){

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
#endif

vec4 NormalMapping(in vec2 uv){
    //Normal mapping in TBN space
    return Phong(uv, normalize(2.0 * texture(texNormalMap, uv).rgb - 1.0));
}

#if defined(USE_PARALLAX_MAPPING)
vec4 ParallaxMapping(in vec2 uv, in vec3 pixelToLightTBN){
    vec3 lightVecTBN = normalize(pixelToLightTBN);
    vec3 viewVecTBN = normalize(_viewDirection);
    
    //Offset, scale and bias
    vec2 vTexCoord = uv + ((texture(texNormalMap, uv).a - 0.5) * parallax_factor * (vec2(viewVecTBN.x, -viewVecTBN.y) / viewVecTBN.z));

    //Normal mapping in TBN space
    return Phong(vTexCoord, normalize(2.0 * texture(texNormalMap, vTexCoord).xyz - 1.0));
}

#endif
#if defined(USE_RELIEF_MAPPING)
vec4 ReliefMapping(in int _light, in vec2 uv){
    vec3 viewVecTBN = normalize(_viewDirection);
    //Size and search starting position in texture space
    vec2 AB = relief_factor * vec2(-viewVecTBN.x, viewVecTBN.y)/viewVecTBN.z;

    float h = ReliefMapping_RayIntersection(uv, AB);
    
    vec2 uv_offset = h * AB;
    
    vec3 p = (dvd_ViewMatrix * _vertexW).xyz;
    vec3 v = normalize(p);
    //Compute light direction
    p += v*h*viewVecTBN.z;	
    
    vec2 planes;
    planes.x = -dvd_zPlanes.y / (dvd_zPlanes.y - dvd_zPlanes.x);
    planes.y = -dvd_zPlanes.y * dvd_zPlanes.x / (dvd_zPlanes.y - dvd_zPlanes.x);

    gl_FragDepth =((planes.x * p.z + planes.y) / -p.z);
    
    return NormalMapping(uv + uv_offset);
}
#endif
