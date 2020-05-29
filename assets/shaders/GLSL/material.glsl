-- Fragment

#if defined(USE_ALBEDO_ALPHA) || defined(USE_OPACITY_MAP)
#   define HAS_TRANSPARENCY
#endif

layout(early_fragment_tests) in;

#include "BRDF.frag"
#include "output.frag"

void main (void) {
    mat4 colourMatrix = dvd_Matrices[DATA_IDX]._colourMatrix;
    vec4 albedo = getAlbedo(colourMatrix, TexCoords);
  
#if !defined(OIT_PASS) && defined(USE_ALPHA_DISCARD)
    float alpha = albedo.a;
    if (alpha < INV_Z_TEST_SIGMA) {
        discard;
    }
#endif

    writeOutput(getPixelColour(albedo, colourMatrix, getNormal(TexCoords), TexCoords));
}
