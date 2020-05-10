#ifndef _PRE_PASS_FRAG_
#define _PRE_PASS_FRAG_

#if !defined(PRE_PASS)
#define PRE_PASS
#endif

#include "materialData.frag"
#include "velocityCalc.frag"
#include "shadowUtils.frag"

#if defined(USE_DEFERRED_NORMALS)
layout(location = TARGET_NORMALS_AND_VELOCITY) out vec4 _normalAndVelocityOut;
//r,g = CSM shadow factor for light ids 0 & 1; b - ssao, a - specular/roughness
layout(location = TARGET_EXTRA) out vec4 _extraDetailsOut;
#endif

void writeOutput(in vec2 uv, in vec3 normal, in float alphaFactor) {
#if defined(USE_ALPHA_DISCARD)
    mat4 colourMatrix = dvd_Matrices[DATA_IDX]._colourMatrix;
    float alpha = getAlbedo(colourMatrix, uv).a;
    if (alpha * alphaFactor < 1.0f - Z_TEST_SIGMA) {
        discard;
    }
#endif

#if defined(USE_DEFERRED_NORMALS)
    _normalAndVelocityOut.rg = packNormal(normalize(normal));
    for (int i = 0; i < 3; ++i) {
        //_extraDetailsOut[i] = getShadowFactor(i);
    }
    _normalAndVelocityOut.ba = velocityCalc();
#endif
}

void writeOutput(in vec2 uv, in vec3 normal) {
    writeOutput(uv, normal, 1.0f);
}

void writeOutput(in vec2 uv, in float alphaFactor) {
    writeOutput(uv, getNormal(uv), alphaFactor);
}

void writeOutput(in vec2 uv) {
    writeOutput(uv, 1.0f);
}

#endif //_PRE_PASS_FRAG_