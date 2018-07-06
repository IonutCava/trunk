-- Vertex

#include "vboInputData.vert"
#include "foliage.vert"
#include "lightInput.cmn"

uniform bool dvd_enableShadowMapping = false;
uniform mat4 dvd_lightProjectionMatrices[MAX_SHADOW_CASTING_LIGHTS];

out vec3 _lightDirection[MAX_LIGHT_COUNT];
out vec4 _shadowCoord[MAX_SHADOW_CASTING_LIGHTS];
out vec3 _normalWV;
out vec4 _vertexWV;
out vec4 _grassColor;

uniform int dvd_lightCount;

void computeLightVectorsPhong(){
    vec3 tmpVec; 

    int i = 0; ///Only the first light for now
    //for(int i = 0; i < MAX_LIGHT_COUNT; i++){
    //	if(dvd_lightCount == i) break;
    vec4 vLightPosMV = dvd_ViewMatrix * dvd_LightSource[dvd_lightIndex[i]]._position;	
    if(vLightPosMV.w == 0.0){ ///<Directional Light
        tmpVec = -vLightPosMV.xyz;					
    }else{///<Omni or spot. Change later if spot
        tmpVec = vLightPosMV.xyz - _vertexWV.xyz;	
    }
    _lightDirection[0] = tmpVec;
}

void main(void){
    computeData();
    
    _normalWV = normalize(dvd_NormalMatrix * dvd_Normal);

    computeFoliageMovementGrass(dvd_Normal, _normalWV, dvd_Vertex);
    _vertexWV = dvd_ViewMatrix * _vertexW; 	   //< ModelView Vertex  

    computeLightVectorsPhong();
    vec3 vLightPosMVTemp = _lightDirection[0];
    float intensity = dot(vLightPosMVTemp.xyz, _normalWV);
    _grassColor = vec4(intensity, intensity, intensity, 1.0);
    _grassColor.a = 1.0 - clamp(length(_vertexWV)/lod_metric, 0.0, 1.0);
            
    gl_Position = dvd_ProjectionMatrix * _vertexWV;
    
    if(dvd_enableShadowMapping) {
        // position multiplied by the light matrix. 
        //The vertex's position from the light's perspective
        _shadowCoord[0] = dvd_lightProjectionMatrices[0] * _vertexW;
    }
}

-- Fragment

in vec2 _texCoord;
in vec3 _normalWV;
in vec4 _vertexWV;
in vec3 _lightDirection[MAX_LIGHT_COUNT];
in vec4 _grassColor;
out vec4 _colorOut;

uniform sampler2D texDiffuse;

///Global NDotL, basically
float iDiffuse;
#include "lightInput.cmn"
#include "shadowMapping.frag"

void main (void){
	
    vec4 cBase = texture(texDiffuse, _texCoord);
	_colorOut.a = min(cBase.a, _grassColor.a);

    int i = 0;
    float cAmbient = dvd_LightSource[dvd_lightIndex[i]]._diffuse.w;
    vec3 cDiffuse = dvd_LightSource[dvd_lightIndex[i]]._diffuse.rgb * _grassColor.rgb;
    vec3 L = normalize(_lightDirection[0]);
    iDiffuse = max(dot(L, _normalWV), 0.0);
    // SHADOW MAPPING
    vec3 vPixPosInDepthMap;
    float shadow = 1.0;
    applyShadowDirectional(0, shadow);
	
    _colorOut.rgb = cAmbient * cBase.rgb + (0.2 + 0.8 * shadow) * cDiffuse * cBase.rgb;
	_colorOut = vec4(1.0);
	_colorOut.rb = vec2(0.0);
}


