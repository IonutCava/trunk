#ifndef _PRE_PASS_FRAG_
#define _PRE_PASS_FRAG_

#if !defined(PRE_PASS)
#define PRE_PASS
#endif

#if defined(USE_ALBEDO_ALPHA) || defined(USE_OPACITY_MAP)
#   define HAS_TRANSPARENCY
#endif

#if defined(USE_ALPHA_DISCARD) || defined(USE_DEFERRED_NORMALS)
#define HAS_PRE_PASS_DATA
#endif //USE_ALPHA_DISCARD || USE_DEFERRED_NORMALS

#if defined(HAS_PRE_PASS_DATA)

#include "materialData.frag"

#if defined(USE_DEFERRED_NORMALS)

layout(location = TARGET_NORMALS_AND_VELOCITY) out vec4 _normalAndVelocityOut;
//r - ssao, b - extra flags
layout(location = TARGET_EXTRA) out vec2 _extraDetailsOut;

#include "velocityCalc.frag"
#endif

void writeOutput(in NodeData nodeData,
                 vec2 uv,
                 vec3 normal,
                 float alphaFactor,
                 float extraFlag)
{
#if defined(USE_ALPHA_DISCARD)
    float alpha = getAlbedo(nodeData._colourMatrix, uv).a;
    if (alpha * alphaFactor < INV_Z_TEST_SIGMA) {
        discard;
    }
#endif
#if defined(USE_DEFERRED_NORMALS)
    _normalAndVelocityOut.rg = packNormal(normalize(normal));
    _normalAndVelocityOut.ba = velocityCalc();
    _extraDetailsOut.rg = vec2(1.0f, extraFlag);
#endif
}
void writeOutput(in NodeData nodeData,
                vec2 uv,
                vec3 normal,
                float alphaFactor)
{
#if defined(HAS_PRE_PASS_DATA)
    writeOutput(nodeData, uv, normal, alphaFactor, 0.0f);
#endif //HAS_PRE_PASS_DATA
}

void writeOutput(in NodeData nodeData,
                 vec2 uv,
                 vec3 normal) 
{
#if defined(HAS_PRE_PASS_DATA)
    writeOutput(nodeData, uv, normal, 1.0f, 0.0f);
#endif //HAS_PRE_PASS_DATA
}

#endif //HAS_PRE_PASS_DATA
#endif //_PRE_PASS_FRAG_