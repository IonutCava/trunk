

-- Vertex

#include "vbInputData.vert"

void main(void){
    computeData();
    VAR._vertexW.xyz += dvd_cameraPosition.xyz;

    VAR._normalWV = normalize(dvd_NormalMatrixWV(VAR.dvd_instanceID) * dvd_Normal);

    gl_Position = dvd_ViewProjectionMatrix * VAR._vertexW;
    gl_Position.z = gl_Position.w - 0.0001;
}

-- Fragment.Display

layout(early_fragment_tests) in;

layout(location = 0) out vec4 _skyColour;
layout(location = 1) out vec2 _normalOut;
layout(location = 2) out vec2 _velocityOut;

layout(binding = TEXTURE_UNIT0) uniform samplerCubeArray texSky;

uniform bool enable_sun;
uniform vec3 sun_vector;
uniform vec3 sun_colour;

#include "utility.frag"

vec3 sunColour(){
    vec3 vert = normalize(_in._vertexW.xyz);
    vec3 sun = normalize(sun_vector);
        
    float day_factor = max(-sun.y, 0.0);
        
    float dotv = max(dot(vert, -sun), 0.0);
    vec3  sun_colour = clamp(sun_colour*1.5, 0.0, 1.0);
    
    float pow_factor = day_factor * 225.0 + 75.0;
    float sun_factor = clamp(pow(dotv, pow_factor), 0.0, 1.0);
        
    return day_factor + sun_colour * sun_factor;
}

void main() {
    vec3 sky_colour = textureLod(texSky, vec4(VAR._vertexW.xyz - dvd_cameraPosition.xyz, 0), 0).rgb;
    _skyColour = vec4(enable_sun ? sky_colour * sunColour() : sky_colour, 1.0);
    _normalOut = packNormal(normalize(VAR._normalWV));
    _velocityOut = vec2(1.0);
}

--Fragment.PrePass

void main() {
}