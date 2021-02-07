-- Fragment.PrePass

#include "prePass.frag"

void main() {
#if defined(USE_ALPHA_DISCARD)
    NodeMaterialData data = dvd_Materials[MATERIAL_IDX];
    const float alpha = getAlbedo(data, VAR._texCoord).a;
    writeGBuffer(alpha, VAR._texCoord, getNormalWV(VAR._texCoord));
#else
    writeGBuffer(1.0f, VAR._texCoord, getNormalWV(VAR._texCoord));
#endif
}

-- Fragment.Shadow

#if defined(USE_ALBEDO_ALPHA) || defined(USE_OPACITY_MAP)
#   define HAS_TRANSPARENCY
#endif

#if defined(HAS_TRANSPARENCY)
#include "materialData.frag"
#endif

void main() {
#if defined(HAS_TRANSPARENCY)
    NodeMaterialData data = dvd_Materials[MATERIAL_IDX];
    if (getAlbedo(data, VAR._texCoord).a < INV_Z_TEST_SIGMA) {
        discard;
    }
#endif
}

--Fragment.Shadow.VSM

#if defined(USE_ALBEDO_ALPHA) || defined(USE_OPACITY_MAP)
#   define HAS_TRANSPARENCY
#endif

#include "vsm.frag"
out vec2 _colourOut;

#if defined(HAS_TRANSPARENCY)
#include "materialData.frag"
#endif


void main() {
#if defined(HAS_TRANSPARENCY)
    const NodeMaterialData data = dvd_Materials[MATERIAL_IDX];
    const vec4 albedo = getAlbedo(data, VAR._texCoord);
    if (albedo .a < INV_Z_TEST_SIGMA) {
        discard;
    }
#endif

    _colourOut = computeMoments();
}