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
    gl_Position = dvd_ViewProjectionMatrix * VAR._vertexW;
}

-- Fragment

#include "BRDF.frag"

#if defined(COMPUTE_TBN)
    #include "bumpMapping.frag"
#endif

out vec4 _colorOut;

//subroutine vec4 MappingRoutineType();

//layout(location = 0) subroutine uniform MappingRoutineType MappingRoutine;

//subroutine(MappingRoutineType)
vec4 mappingFlat(){
    return getPixelColor(VAR._texCoord, VAR._normalWV);
}

#if defined(COMPUTE_TBN)
//subroutine(MappingRoutineType)
vec4 mappingNormal(){
    return getPixelColor(VAR._texCoord, getTBNNormal(VAR._texCoord));
}

//subroutine(MappingRoutineType)
vec4 mappingRelief(){
    return ReliefMapping(bumpMapLightID, VAR._texCoord);
}

//subroutine(MappingRoutineType)
vec4 mappingParallax(){
    return ParallaxMapping(bumpMapLightID, VAR._texCoord);
}
#endif

void main (void){
    //_colorOut = ToSRGB(applyFog(MappingRoutine()));
#if defined(COMPUTE_TBN)
#    if defined(USE_PARALLAX_MAPPING)
    _colorOut = ToSRGB(applyFog(ParallaxMapping()));
#    elif defined(USE_RELIEF_MAPPING)
    _colorOut = ToSRGB(applyFog(ReliefMapping()));
#    else
    _colorOut = ToSRGB(applyFog(mappingNormal()));
#    endif
#else
    _colorOut = ToSRGB(applyFog(mappingFlat()));
#endif
}   