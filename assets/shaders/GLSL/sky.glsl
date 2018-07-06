-- Vertex

out vec3 _vertex;
out vec3 _normal;

vec3 UNPACK_FLOAT(in float value) {
    return (fract(vec3(1.0, 256.0, 65536.0) * value)* 2.0) - 1.0;
}

void main(void){
    _vertex = normalize(inVertexData);
    _normal = UNPACK_FLOAT(inNormalData);
    gl_Position = dvd_ViewProjectionMatrix * vec4(inVertexData + dvd_cameraPosition.xyz, 1.0);
    gl_Position.z = gl_Position.w -0.00001; //fix to far plane.
}

-- Fragment

in vec3 _vertex;
in vec3 _normal;
out vec4 _skyColor;

uniform bool isDepthPass;
uniform bool enable_sun;
uniform vec3 sun_vector;
uniform vec3 sun_color;
uniform samplerCubeArray texSky;

#include "utility.frag"

vec3 sunColor(){
    vec3 vert = normalize(_vertex);
    vec3 sun = normalize(sun_vector);
        
    float day_factor = max(-sun.y, 0.0);
        
    float dotv = max(dot(vert, -sun), 0.0);
    vec3  sun_color = clamp(sun_color*1.5, 0.0, 1.0);
    
    float pow_factor = day_factor * 225.0 + 75.0;
    float sun_factor = clamp(pow(dotv, pow_factor), 0.0, 1.0);
        
    return day_factor + sun_color * sun_factor;
}

void main (void){

    if (isDepthPass) {
        _skyColor.rgb = normalize(dvd_NormalMatrix() * _normal);
        _skyColor.a = 1.0;
    } else {
        vec3 sky_color = texture(texSky, vec4(_vertex.xyz, 0)).rgb;
        _skyColor = vec4(ToSRGB(enable_sun ? sky_color * sunColor() : sky_color), 1.0);
    }
}
