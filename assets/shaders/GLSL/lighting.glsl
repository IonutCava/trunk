-- Vertex

#include "vbInputData.vert"
#include "lightingDefaults.vert"

#if defined(ADD_FOLIAGE)
#include "foliage.vert"
#endif
void main(void){

    computeData();

#if defined(ADD_FOLIAGE) && defined(IS_TREE)
    computeFoliageMovementTree(dvd_Vertex);
#endif
    
    computeLightVectors();

    //Compute the final vert position
    gl_Position = dvd_ViewProjectionMatrix * _vertexW;
}

-- Fragment

#include "phong_lighting.frag"

#if defined(COMPUTE_TBN)
    #include "bumpMapping.frag"
#endif

out vec4 _colorOut;

//subroutine vec4 MappingRoutineType();

//layout(location = 0) subroutine uniform MappingRoutineType MappingRoutine;

//subroutine(MappingRoutineType)
vec4 mappingFlat(){
	return Phong(_texCoord, _normalWV);
}

#if defined(COMPUTE_TBN)
//subroutine(MappingRoutineType)
vec4 mappingNormal(){
	return Phong(_texCoord, normalize(2.0 * texture(texNormalMap, _texCoord).rgb - 1.0));
}

//subroutine(MappingRoutineType)
vec4 mappingRelief(){
	return ReliefMapping(bumpMapLightId,_texCoord);
}

//subroutine(MappingRoutineType)
vec4 mappingParallax(){
    vec3 lightDir = vec3(0.0);
    switch (uint(dvd_LightSource[bumpMapLightId]._position.w)){
        case LIGHT_DIRECTIONAL      : lightDir = -normalize(dvd_LightSource[bumpMapLightId]._position.xyz); break;
        case LIGHT_OMNIDIRECTIONAL  : 
        case LIGHT_SPOT             : lightDir = normalize(_viewDirection + dvd_LightSource[bumpMapLightId]._position.xyz); break;
    };

	return ParallaxMapping(_texCoord, lightDir);
}
#endif

void main (void){
	//_colorOut = applyFog(MappingRoutine());
#if defined(COMPUTE_TBN)
#	if defined(USE_PARALLAX_MAPPING)
    _colorOut = applyFog(ParallaxMapping());
#	elif defined(USE_RELIEF_MAPPING)
    _colorOut = applyFog(ReliefMapping());
#	else
    _colorOut = applyFog(mappingNormal());
#	endif
#else
	_colorOut = applyFog(mappingFlat());
#endif
}