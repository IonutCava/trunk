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

#if defined(USE_ALBEDO_ALPHA) || defined(USE_OPACITY_MAP)
#   define HAS_TRANSPARENCY
#endif

layout(early_fragment_tests) in;

#include "BRDF.frag"
#include "output.frag"

void main (void) {
    updateTexCoord();

    vec3 normal = getNormal();

    mat4 colourMatrix = dvd_Matrices[VAR.dvd_baseInstance]._colourMatrix;
    vec4 albedo = getAlbedo(colourMatrix);
#if defined(USE_ALPHA_DISCARD)
    if (albedo.a < 1.0 - Z_TEST_SIGMA) {
        discard;
    }
#endif

    writeOutput(getPixelColour(albedo, colourMatrix, normal));
}
