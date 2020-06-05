#ifndef _PRE_PASS_FRAG_
#define _PRE_PASS_FRAG_

#if !defined(PRE_PASS)
#define PRE_PASS
#endif

#include "materialData.frag"

#if defined(USE_DEFERRED_NORMALS)
layout(location = TARGET_NORMALS_AND_VELOCITY) out vec4 _normalAndVelocityOut;
//r,g = TBN view direction,  b - ssao, a - specular/roughness
layout(location = TARGET_EXTRA) out vec4 _extraDetailsOut;

#include "velocityCalc.frag"
//#include "shadowUtils.frag"
#endif

void writeOutput(in NodeData data, in vec2 uv, in vec3 normal, in vec3 tbnViewDirection, in float alphaFactor) {
#if defined(USE_ALPHA_DISCARD)
    float alpha = getAlbedo(data._colourMatrix, uv).a;
    if (alpha * alphaFactor < INV_Z_TEST_SIGMA) {
        discard;
    }
#endif

#if defined(USE_DEFERRED_NORMALS)
    _normalAndVelocityOut.rg = packNormal(normalize(normal));
    _normalAndVelocityOut.ba = velocityCalc();
    _extraDetailsOut.rg = packNormal(normalize(tbnViewDirection));
#endif
}

void writeOutput(in NodeData data, in vec2 uv, in vec3 normal, in vec3 tbnViewDirection) {
#if defined(USE_ALPHA_DISCARD) || defined(USE_DEFERRED_NORMALS)
    writeOutput(data, uv, normal, tbnViewDirection, 1.0f);
#endif
}

void writeOutput(in NodeData data, in vec2 uv, in vec3 normal) {
#if defined(USE_ALPHA_DISCARD) || defined(USE_DEFERRED_NORMALS)
    writeOutput(data, uv, normal, getTBNViewDirection(), 1.0f);
#endif
}

void writeOutput(in NodeData data, in vec2 uv, in float alphaFactor) {
#if defined(USE_ALPHA_DISCARD) || defined(USE_DEFERRED_NORMALS)
    writeOutput(data, uv, getNormalWV(uv), getTBNViewDirection(), alphaFactor);
#endif
}

void writeOutput(in NodeData data, in vec2 uv) {
#if defined(USE_ALPHA_DISCARD) || defined(USE_DEFERRED_NORMALS)
    writeOutput(data, uv, getNormalWV(uv), getTBNViewDirection(), 1.0f);
#endif
}

#endif //_PRE_PASS_FRAG_