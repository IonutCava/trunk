-- Vertex

#include "vbInputData.vert"

void main(void){
    computeData();
    VAR._normalWV = normalize(dvd_NormalMatrixWV(VAR.dvd_drawID) * dvd_Normal);
    VAR._vertexVelocity = vec2(0.0, 0.0);

    gl_Position = vec4(dvd_ViewProjectionMatrix * VAR._vertexW).xyzz;
    gl_Position.w += 0.0001;
}

/*-- Fragment.PrePass

layout(location = 0) out vec4 _skyColour;

void main() {
    _skyColour = vec4(1.0);
}*/

-- Fragment.Display

layout(location = 0) out vec4 _skyColour;
layout(location = 1) out vec2 _normalOut;
layout(location = 2) out vec2 _velocityOut;

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
    vec3 sky_colour = textureLod(texSky, vec4(VAR._vertexW.xyz, 0), 0).rgb;
    _skyColour = vec4(ToSRGB(enable_sun ? sky_colour * sunColour() : sky_colour), 1.0);
    _normalOut = packNormal(normalize(VAR._normalWV));
    _velocityOut = VAR._vertexVelocity;

    //_skyColour = vec4(VAR._vertexW.xyz, 1.0);//textureLod(texSky, vec4(0, 1, 0, 0), 0).rgb
}
