-- Vertex

#include "vbInputData.vert"
#include "foliage.vert"
#include "lightInput.cmn"

in  vec3 dvd_cameraPosition;
out vec4 _grassColor;

uniform int dvd_lightCount;

uniform float grassVisibilityRange;

void computeLightVectorsPhong(){
    vec4 tmpVec; 

    int i = 0; ///Only the first light for now
    //for(int i = 0; i < MAX_LIGHTS_PER_NODE; i++){
    //	if(dvd_lightCount == i) break;
    vec4 vLightPosMV = dvd_ViewMatrix * dvd_LightSource[dvd_lightIndex[i]]._position;	
    if(vLightPosMV.w == 0.0){ ///<Directional Light
        tmpVec = -vLightPosMV;					
    }else{///<Omni or spot. Change later if spot
        tmpVec = vLightPosMV - dvd_Vertex;	
    }
    _lightDirection[0] = tmpVec.xyz;

}

void main(void){
    computeData();
    _normalWV = normalize(dvd_Normal);

    computeFoliageMovementGrass(_normalWV, dvd_Vertex);

    _grassColor.a = clamp(grassVisibilityRange / length(dvd_cameraPosition - dvd_Vertex.xyz), 0.0, 1.0);
    
    computeLightVectorsPhong();
    float intensity = dot(_lightDirection[0].xyz, _normalWV);

    _grassColor.rgb = vec3(intensity);
          
    gl_Position = dvd_ViewProjectionMatrix * dvd_Vertex;
}

-- Fragment

in vec2 _texCoord;
in vec4 _grassColor;
in vec4 _vertexW;

out vec4 _colorOut;

uniform sampler2D texDiffuse;

#include "lightInput.cmn"
#include "shadowMapping.frag"
#include "lightingDefaults.frag"

void main (void){
    
    vec4 cBase = texture(texDiffuse, _texCoord);

    int i = 0;
    float cAmbient = dvd_LightSource[dvd_lightIndex[i]]._diffuse.w;
    vec3 cDiffuse = dvd_LightSource[dvd_lightIndex[i]]._diffuse.rgb * _grassColor.rgb;
    float iDiffuse = max(dot(normalize(_lightDirection[0]), _normalWV), 0.0);

    // SHADOW MAPPING
    float shadow = 1.0;
    applyShadowDirectional(0, shadow);
    
    _colorOut = vec4(cAmbient * cBase.rgb + cDiffuse * cBase.rgb *(0.2 + 0.8 * shadow), cBase.a * _grassColor.a);
}


