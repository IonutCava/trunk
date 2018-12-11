-- Vertex

#include "vbInputData.vert"
#include "lightingDefaults.vert"

#if defined(ADD_FOLIAGE)
#include "foliage.vert"
#endif

#ifndef USE_COLOURDE_WOIT
//#define USE_COLOURED_WOIT
#endif //USE_COLOURED_WOIT

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

#if defined(USE_OPACITY_DIFFUSE) || defined(USE_OPACITY_MAP) || defined(USE_OPACITY_DIFFUSE_MAP)
#   define HAS_TRANSPARENCY
#endif

#if !defined(HAS_TRANSPARENCY)
layout(early_fragment_tests) in;
#endif

#include "BRDF.frag"

#if defined(COMPUTE_TBN)
    #include "bumpMapping.frag"
#endif

#include "velocityCalc.frag"

#include "output.frag"
//subroutine vec4 MappingRoutineType();

//layout(location = 0) subroutine uniform MappingRoutineType MappingRoutine;

//subroutine(MappingRoutineType)
vec4 mappingFlat(){
    setProcessedNormal(VAR._normalWV);
    return getPixelColour();
}

#if defined(COMPUTE_TBN)
//subroutine(MappingRoutineType)
vec4 mappingNormal(){
    setProcessedNormal(getTBNMatrix() * getBump(VAR._texCoord));
    return mix(getPixelColour(),
               mappingFlat(),
               vec4(dvd_lodLevel > 1));
}

//subroutine(MappingRoutineType)
vec4 mappingRelief(){
    return mix(ReliefMapping(VAR._texCoord),
               mappingFlat(),
               vec4(dvd_lodLevel > 1));
}

//subroutine(MappingRoutineType)
vec4 mappingParallax(){
    return mix(ParallaxMapping(VAR._texCoord),
               mappingFlat(),
               vec4(dvd_lodLevel > 1));
}
#endif

vec4 getFinalPixelColour() {
//return MappingRoutine();
#if defined(COMPUTE_TBN)
    bumpInit();
#    if defined(USE_PARALLAX_MAPPING)
    return mappingParallax();
#    elif defined(USE_RELIEF_MAPPING)
    return mappingRelief();
#    else
    return mappingNormal();
#    endif
#else
   return mappingFlat();
#endif
}

void main (void){
    vec4 colourOut = vec4(0.0);

#if defined(_DEBUG)
    if (dvd_NormalsOnly) {
        colourOut.rgb = getProcessedNormal();
    } else {
        colourOut = getFinalPixelColour();
    }
#else
    colourOut = getFinalPixelColour();
#endif

    writeOutput(colourOut, packNormal(getProcessedNormal()));
}
