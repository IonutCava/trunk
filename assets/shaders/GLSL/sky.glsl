-- Vertex

out vec3 _vertex;

void main(void){
    _vertex = normalize(inVertexData);
    gl_Position = dvd_ViewProjectionMatrix * vec4(inVertexData + dvd_cameraPosition.xyz, 1.0);
    gl_Position.z = gl_Position.w -0.00001; //fix to far plane.
}

-- Fragment

in vec3 _vertex;
out vec4 _skyColor;

uniform bool enable_sun;
uniform vec3 sun_vector;
uniform vec3 sun_color;
uniform samplerCube texSky;

#include "lightingDefaults.frag"

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

    vec3 sky_color = texture(texSky, _vertex.xyz).rgb;
    _skyColor = applyGamma(vec4(enable_sun ? sky_color * sunColor() : sky_color, 1.0));
}
