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
in vec4 _vertexW;
in float dvd_ClipDistance[MAX_CLIP_PLANES];

uniform float opacity = 1.0;
uniform sampler2D texOpacityMap;
uniform sampler2D texDiffuse0;
uniform mat4      material;

out vec4 _colorOut;

void main(){

#if defined(USE_OPACITY_DIFFUSE)
    if(material[1].w < ALPHA_DISCARD_THRESHOLD) discard;
#elif defined(USE_OPACITY)
    if(opacity< ALPHA_DISCARD_THRESHOLD) discard;
#elif defined(USE_OPACITY_MAP)
    // discard material if it is bellow opacity threshold
    if(texture(texOpacityMap, _texCoord).a < ALPHA_DISCARD_THRESHOLD) discard;
#elif defined(USE_OPACITY_DIFFUSE_MAP)
    // discard material if it is bellow opacity threshold
    if(texture(texDiffuse0, _texCoord).a < ALPHA_DISCARD_THRESHOLD) discard;
#endif

    float depth = (_vertexWVP.z / _vertexWVP.w) * 0.5 + 0.5;
    // Adjusting moments (this is sort of bias per pixel) using partial derivative
    float dx = dFdx(depth);
    float dy = dFdy(depth);
    
    _colorOut = vec4(depth, (depth * depth) + 0.25 * (dx * dx + dy * dy), 0.0, 0.0);
}


-- Fragment.PrePass

in vec2 _texCoord;
in vec4 _vertexWVP;
in vec4 _vertexW;
in float dvd_ClipDistance[MAX_CLIP_PLANES];

uniform float opacity = 1.0;
uniform sampler2D texOpacityMap;
uniform sampler2D texDiffuse0;
uniform mat4      material;

out vec4 _colorOut;

void main(){

#if defined(USE_OPACITY_DIFFUSE)
    if(material[1].w < ALPHA_DISCARD_THRESHOLD) discard;
#elif defined(USE_OPACITY)
    if(opacity< ALPHA_DISCARD_THRESHOLD) discard;
#elif defined(USE_OPACITY_MAP)
    // discard material if it is bellow opacity threshold
    if(texture(texOpacityMap, _texCoord).a < ALPHA_DISCARD_THRESHOLD) discard;
#elif defined(USE_OPACITY_DIFFUSE_MAP)
    // discard material if it is bellow opacity threshold
    if(texture(texDiffuse0, _texCoord).a < ALPHA_DISCARD_THRESHOLD) discard;
#endif

    _colorOut = vec4(_vertexWVP.w, 0.0, 0.0, 1.0);
}