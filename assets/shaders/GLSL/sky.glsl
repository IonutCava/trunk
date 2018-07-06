-- Vertex
in vec3  inVertexData;

uniform mat4 dvd_WorldViewProjectionMatrix;

layout(std140) uniform dvd_MatrixBlock
{
    mat4 dvd_ProjectionMatrix;
    mat4 dvd_ViewMatrix;
	mat4 dvd_ViewProjectionMatrix;
};

out vec3 _vertex;

void main(void){
    vec4 dvd_Vertex = vec4(inVertexData,1.0);
    _vertex = normalize(dvd_Vertex.xyz);
    gl_Position = dvd_WorldViewProjectionMatrix * dvd_Vertex;
	gl_Position.z = gl_Position.w -0.00001; //fix to far plane.
}

-- Fragment

in vec3 _vertex;
out vec4 _skyColor;

uniform bool enable_sun;
uniform vec3 sun_vector;

uniform samplerCube texSky;

#include "lightInput.cmn"

void main (void){

    vec4 sky_color = texture(texSky, _vertex.xyz);
    _skyColor = sky_color;

    if(enable_sun){

        vec3 vert = normalize(_vertex);
        vec3 sun = normalize(sun_vector);
        
        float day_factor = max(-sun.y, 0.0);
        
        float dotv = max(dot(vert, -sun), 0.0);
        vec3 sun_color = clamp(dvd_LightSource[0]._diffuse.rgb*1.5, 0.0, 1.0);
    
        float pow_factor = day_factor * 175.0 + 25.0;
        float sun_factor = clamp(pow(dotv, pow_factor), 0.0, 1.0);
        
        _skyColor *= day_factor + vec4(sun_color, 1.0) * sun_factor;
    
    }

    _skyColor.a = 1.0;
}
