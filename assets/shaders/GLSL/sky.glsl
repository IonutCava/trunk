-- Vertex

#include "vbInputData.vert"

void main(void){
    computeData();
    VAR._vertexW.xyz += dvd_cameraPosition.xyz;

    VAR._normalWV = normalize(dvd_NormalMatrixWV(VAR.dvd_baseInstance) * dvd_Normal);

    gl_Position = dvd_ViewProjectionMatrix * VAR._vertexW;
    gl_Position.z = gl_Position.w - 0.000001f;
}

-- Fragment.Display

layout(early_fragment_tests) in;

layout(binding = TEXTURE_UNIT0) uniform samplerCubeArray texSky;

uniform bool enable_sun;
uniform vec3 sun_vector;
uniform vec3 sun_colour;

#include "utility.frag"
#include "output.frag"

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
    vec4 skyColour = vec4(enable_sun ? sky_colour * sunColour() : sky_colour, 1.0);
    writeOutput(skyColour);
}

--Fragment.PrePass

#include "prePass.frag"

void main() {
    outputNoVelocity(VAR._texCoord, 1.0f);
}