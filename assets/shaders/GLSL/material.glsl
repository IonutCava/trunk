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

#include "output.frag"

vec4 mappingFlat(){
    setProcessedNormal(VAR._normalWV);
    return getPixelColour();
}


vec4 getFinalPixelColour() {
#if defined(COMPUTE_TBN)
    if (dvd_lodLevel == 0) {
#   if defined(USE_PARALLAX_MAPPING)
        return ParallaxMapping(VAR._texCoord, 0);
#   elif defined(USE_RELIEF_MAPPING)
        return vec4(1.0, 0.0, 0.0, 1.0);
#   else
        setProcessedNormal(getTBNMatrix() * getBump(VAR._texCoord));
        return getPixelColour();
#   endif
    }
#endif

   return mappingFlat();
}

void main (void) {
    vec4 colour = getFinalPixelColour();
    writeOutput(colour, packNormal(getProcessedNormal()));
}
