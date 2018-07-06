-- Vertex

#include "vbInputData.vert"

void main(void){
    computeData();
    VAR._normalWV = normalize(dvd_NormalMatrixWV() * dvd_Normal);
    gl_Position = vec4(dvd_ViewProjectionMatrix * VAR._vertexW).xyzz;
    gl_Position.w += 0.01;
}

-- Fragment


#if defined (IS_PRE_PASS)
layout(location = 1) out vec3 _normalWV;
#else
layout(location = 0) out vec4 _skyColor;
#endif

layout(binding = TEXTURE_UNIT0) uniform samplerCubeArray texSky;

#if !defined (IS_PRE_PASS)
uniform bool enable_sun;
uniform vec3 sun_vector;
uniform vec3 sun_color;

#include "utility.frag"

vec3 sunColor(){
    vec3 vert = normalize(f_in._vertexW.xyz);
    vec3 sun = normalize(sun_vector);
        
    float day_factor = max(-sun.y, 0.0);
        
    float dotv = max(dot(vert, -sun), 0.0);
    vec3  sun_color = clamp(sun_color*1.5, 0.0, 1.0);
    
    float pow_factor = day_factor * 225.0 + 75.0;
    float sun_factor = clamp(pow(dotv, pow_factor), 0.0, 1.0);
        
    return day_factor + sun_color * sun_factor;
}
#endif

void main (void){
#if defined (IS_PRE_PASS)
    _normalWV = normalize(f_in._normalWV);
#else
    vec3 sky_color = texture(texSky, vec4(f_in._vertexW.xyz, 0)).rgb;
    _skyColor = vec4(ToSRGB(enable_sun ? sky_color * sunColor() : sky_color), 1.0);
#endif
}
