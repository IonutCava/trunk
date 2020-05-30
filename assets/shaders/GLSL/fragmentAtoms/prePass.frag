#ifndef _PRE_PASS_FRAG_
#define _PRE_PASS_FRAG_

#if !defined(PRE_PASS)
#define PRE_PASS
#endif

#include "materialData.frag"

#if defined(USE_DEFERRED_NORMALS)
layout(location = TARGET_NORMALS_AND_VELOCITY) out vec4 _normalAndVelocityOut;
//r,g = CSM shadow factor for light ids 0 & 1; b - ssao, a - specular/roughness
layout(location = TARGET_EXTRA) out vec4 _extraDetailsOut;

#include "velocityCalc.frag"
//#include "shadowUtils.frag"
#endif

void writeOutput(in NodeData data, in vec2 uv, in vec3 normal, in float alphaFactor) {
#if defined(USE_ALPHA_DISCARD)
    float alpha = getAlbedo(data._colourMatrix, uv).a;
    if (alpha * alphaFactor < INV_Z_TEST_SIGMA) {
        discard;
    }
#endif

#if defined(USE_DEFERRED_NORMALS)
    _normalAndVelocityOut.rg = packNormal(normalize(normal));
    //for (int i = 0; i < 3; ++i) {
        //_extraDetailsOut[i] = getShadowFactor(i, 0);
    //}
    _extraDetailsOut = vec4(1.0f, 1.0f, 1.0f, 1.0f);
    _normalAndVelocityOut.ba = velocityCalc();
#endif
}

void writeOutput(in NodeData data, in vec2 uv, in vec3 normal) {
#if defined(USE_ALPHA_DISCARD) || defined(USE_DEFERRED_NORMALS)
    writeOutput(data, uv, normal, 1.0f);
#endif
}

void writeOutput(in NodeData data, in vec2 uv, in float alphaFactor) {
#if defined(USE_ALPHA_DISCARD) || defined(USE_DEFERRED_NORMALS)
    writeOutput(data, uv, getNormal(uv), alphaFactor);
#endif
}

void writeOutput(in NodeData data, in vec2 uv) {
#if defined(USE_ALPHA_DISCARD) || defined(USE_DEFERRED_NORMALS)
    writeOutput(data, uv, getNormal(uv), 1.0f);
#endif
}

#endif //_PRE_PASS_FRAG_