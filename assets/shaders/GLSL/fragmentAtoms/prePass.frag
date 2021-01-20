#ifndef _PRE_PASS_FRAG_
#define _PRE_PASS_FRAG_

#if !defined(PRE_PASS)
#define PRE_PASS
#endif

#if defined(USE_ALPHA_DISCARD) || defined(USE_OPACITY_MAP)
#   define HAS_TRANSPARENCY
#endif

#if defined(USE_ALPHA_DISCARD) || defined(USE_DEFERRED_NORMALS)
#define HAS_PRE_PASS_DATA
#endif //USE_ALPHA_DISCARD || USE_DEFERRED_NORMALS

#if defined(HAS_PRE_PASS_DATA)

#include "materialData.frag"

#if defined(USE_DEFERRED_NORMALS)
layout(location = TARGET_NORMALS_AND_VELOCITY) out vec4 _normalAndVelocityOut;
//r - ssao, b - excludeFromFog
layout(location = TARGET_EXTRA) out vec2 _extraDetailsOut;
#endif //USE_DEFERRED_NORMALS

#include "velocityCalc.frag"

void writeOutput(in float albedoAlpha, in vec2 uv, in vec3 normal)
{
#if defined(USE_ALPHA_DISCARD)
    if (albedoAlpha < INV_Z_TEST_SIGMA) {
        discard;
    }
#endif //USE_ALPHA_DISCARD
#if defined(USE_DEFERRED_NORMALS)
    _normalAndVelocityOut.ba = velocityCalc();
    _normalAndVelocityOut.rg = packNormal(normalize(normal));
#endif //USE_DEFERRED_NORMALS
}

void writeOutput(in float albedoAlpha, in vec2 uv, in vec3 normal, in float extraFlag) {
#if defined(USE_DEFERRED_NORMALS)
    _extraDetailsOut.rg = vec2(1.0f, extraFlag);
#endif //USE_DEFERRED_NORMALS

    writeOutput(albedoAlpha, uv, normal);
}

#else //HAS_PRE_PASS_DATA

#define writeOutput(albedoAlpha, uv, normal)

#endif //HAS_PRE_PASS_DATA

#endif //_PRE_PASS_FRAG_