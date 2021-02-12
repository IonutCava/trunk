-- Fragment

#if defined(USE_ALBEDO_ALPHA) || defined(USE_OPACITY_MAP)
#   define HAS_TRANSPARENCY
#endif

layout(early_fragment_tests) in;

#include "BRDF.frag"
#include "output.frag"

void main (void) {
    NodeMaterialData data = dvd_Materials[MATERIAL_IDX];

    vec4 albedo = getAlbedo(data, VAR._texCoord);
  
#if !defined(OIT_PASS) && defined(USE_ALPHA_DISCARD)
    float alpha = albedo.a;
    if (alpha < INV_Z_TEST_SIGMA) {
        discard;
    }
#endif
    const vec3 normalWV = getNormalWV(VAR._texCoord);
    vec2 MetalnessRoughness = vec2(0.f, 1.f);
    const vec4 rgba = getPixelColour(albedo, data, normalWV, VAR._texCoord, 0u, MetalnessRoughness);
    writeScreenColour(rgba, normalWV, MetalnessRoughness);
}
