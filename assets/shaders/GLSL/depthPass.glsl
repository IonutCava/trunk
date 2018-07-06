-- Vertex
#include "vboInputData.vert"
   
#if defined(USE_GPU_SKINNING)
#include "boneTransforms.vert"
#endif

out vec4 _vertexWV;

void main(void){

    computeData();
    
    #if defined(USE_GPU_SKINNING)
     	applyBoneTransforms(dvd_Vertex, dvd_Normal);
 	#endif

    // Transformed position 
    _vertexWV = dvd_WorldViewMatrix * dvd_Vertex;
    //Compute the final vert position
    gl_Position = dvd_ProjectionMatrix * _vertexWV;
}

-- Fragment

in vec2 _texCoord;
in vec4 _vertexWV;

//Opacity and specular maps
uniform float opacity;
#if defined(USE_OPACITY_MAP)
//Opacity and specular maps
uniform sampler2D texOpacityMap;
#endif

out vec4 _colorOut;

void main(){

    float opacityValue = opacity;
#if defined(USE_OPACITY_MAP)
    opacityValue = 1.1 - texture(texOpacityMap, _texCoord).a;
#endif

    /*if(opacityValue < ALPHA_DISCARD_THRESHOLD)
        gl_FragDepth = 1.0; //Discard for depth, basically*/

    float depth = (_vertexWV.z / _vertexWV.w) * 0.5 + 0.5;
    // Adjusting moments (this is sort of bias per pixel) using partial derivative
    float dx = dFdx(depth);
    float dy = dFdy(depth);

    _colorOut = vec4(depth, 0.0, 0.0, (depth * depth) + 0.25 * (dx * dx + dy * dy));
}