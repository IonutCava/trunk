-- Vertex

#include "vbInputData.vert"

void main(void){
    computeData();
    VAR._normalWV = normalize(dvd_NormalMatrixWV(VAR.dvd_drawID) * dvd_Normal);
    gl_Position = vec4(dvd_ViewProjectionMatrix * VAR._vertexW).xyzz;
    gl_Position.w += 0.0001;
}

-- Fragment.Display

layout(location = 0) out vec4 _skyColour;
layout(location = 1) out vec3 _normalOut;

layout(binding = TEXTURE_UNIT0) uniform samplerCubeArray texSky;

uniform bool enable_sun;
uniform vec3 sun_vector;
uniform vec3 sun_colour;

#include "utility.frag"

vec3 sunColour(){
    vec3 vert = normalize(f_in._vertexW.xyz);
    vec3 sun = normalize(sun_vector);
        
    float day_factor = max(-sun.y, 0.0);
        
    float dotv = max(dot(vert, -sun), 0.0);
    vec3  sun_colour = clamp(sun_colour*1.5, 0.0, 1.0);
    
    float pow_factor = day_factor * 225.0 + 75.0;
    float sun_factor = clamp(pow(dotv, pow_factor), 0.0, 1.0);
        
    return day_factor + sun_colour * sun_factor;
}

void main() {
    vec3 sky_colour = texture(texSky, vec4(f_in._vertexW.xyz, 0)).rgb;
    _skyColour = vec4(ToSRGB(enable_sun ? sky_colour * sunColour() : sky_colour), 1.0);
    _normalOut = normalize(f_in._normalWV);
}
