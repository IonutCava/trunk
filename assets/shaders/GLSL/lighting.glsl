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
#include "bumpMapping.frag"

out vec4 _colorOut;

//subroutine vec4 MappingRoutineType();

//layout(location = 0) subroutine uniform MappingRoutineType MappingRoutine;

//subroutine(MappingRoutineType)
vec4 mappingFlat(){
	return Phong(_texCoord, _normalWV);
}

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
	return ParallaxMapping(_texCoord, _lightInfo._lightDirection[bumpMapLightId]);
}

void main (void){
	//vec4 color = MappingRoutine();
#if defined(COMPUTE_TBN)
#	if defined(USE_PARALLAX_MAPPING)
    vec4 color = ParallaxMapping();
#	elif defined(USE_RELIEF_MAPPING)
    vec4 color = ReliefMapping();
#	else
    vec4 color = mappingNormal();
#	endif
#else
	vec4 color = mappingFlat();
#endif
    applyFog(color);

    _colorOut = color;
}