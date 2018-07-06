-- Vertex

#include "vbInputData.vert"

out vec4 _vertex;
out vec3 _normal;

void main(void){
    computeData();
    _normal = dvd_Normal;
    _vertex = dvd_Vertex;
    gl_Position = dvd_ViewProjectionMatrix * vec4(_vertex.xyz + dvd_cameraPosition.xyz, 1.0);
    gl_Position.z = gl_Position.w -0.00001; //fix to far plane.
}

-- Fragment

in vec4 _vertex;
in vec3 _normal;
out vec4 _skyColor;

uniform bool isDepthPass;
uniform bool enable_sun;
uniform vec3 sun_vector;
uniform vec3 sun_color;
uniform samplerCubeArray texSky;

#include "utility.frag"

vec3 sunColor(){
    vec3 vert = normalize(_vertex.xyz);
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
        _skyColor.rgb = normalize(dvd_NormalMatrixWV() * _normal);
        _skyColor.a = 1.0;
    } else {
        vec3 sky_color = textureLod(texSky, vec4(_vertex.xyz, 0), 0).rgb;
        _skyColor = vec4(ToSRGB(enable_sun ? sky_color * sunColor() : sky_color), 1.0);
    }
}
