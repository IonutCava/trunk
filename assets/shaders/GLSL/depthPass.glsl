-- Vertex
#include "vboInputData.vert"
   
out vec4 _vertexWVP;

void main(void){

    computeData();
    
    // Transformed position 
    _vertexWVP = dvd_WorldViewProjectionMatrix * dvd_Vertex;
    //Compute the final vert position
    gl_Position = _vertexWVP;
}

-- Fragment

in vec2 _texCoord;
in vec4 _vertexWVP;

//Opacity and specular maps
uniform float opacity = 1.0;
#if defined(USE_OPACITY_MAP)
//Opacity and specular maps
uniform sampler2D texOpacityMap;
#endif

out vec4 _colorOut;

void main(){
    
    float opacityValue = opacity;

#if defined(USE_OPACITY_MAP)
    // discard material if it is bellow opacity threshold
    if(texture(texOpacityMap, _texCoord).a < ALPHA_DISCARD_THRESHOLD) discard;
#else
    if(opacity< ALPHA_DISCARD_THRESHOLD) discard;
#endif

    float depth = 0.5 * (_vertexWVP.z / _vertexWVP.w) + 0.5;
    // Adjusting moments (this is sort of bias per pixel) using partial derivative
    float dx = dFdx(depth);
    float dy = dFdy(depth);
    
    _colorOut = vec4(depth, depth * depth + 0.25 * (dx * dx + dy * dy), 0.0, 1.0);
}