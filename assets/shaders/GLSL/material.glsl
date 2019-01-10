-- Vertex

#include "vbInputData.vert"
#include "lightingDefaults.vert"

void main(void){

    computeData();
    computeLightVectors();

    //Compute the final vert position
    gl_Position = dvd_ViewProjectionMatrix * VAR._vertexW;

}

-- Fragment

#if defined(USE_OPACITY_DIFFUSE) || defined(USE_OPACITY_MAP) || defined(USE_OPACITY_DIFFUSE_MAP)
#   define HAS_TRANSPARENCY
#endif

layout(early_fragment_tests) in;

#include "BRDF.frag"

#if defined(COMPUTE_TBN)
    #include "bumpMapping.frag"
#endif

#include "velocityCalc.frag"

#include "output.frag"

vec4 mappingFlat(){
    setProcessedNormal(VAR._normalWV);
    return getPixelColour();
}


vec4 getFinalPixelColour() {

#if defined(COMPUTE_TBN)
#   if defined(USE_PARALLAX_MAPPING)
    return mix(ParallaxMapping(VAR._texCoord, 0), mappingFlat(), vec4(dvd_lodLevel > 1));
#    elif defined(USE_RELIEF_MAPPING)
    return mix(ReliefMapping(VAR._texCoord), mappingFlat(), vec4(dvd_lodLevel > 1));
#    else
    setProcessedNormal(getTBNMatrix() * getBump(VAR._texCoord));
    return mix(getPixelColour(), mappingFlat(), vec4(dvd_lodLevel > 1));
#    endif
#else
   return mappingFlat();
#endif
}

void main (void) {
    writeOutput(getFinalPixelColour(), packNormal(getProcessedNormal()));
}
