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
//r,g,b = CSM shadow factor for light ids 0, 1 & 2; a - specular/roughness
layout(location = TARGET_EXTRA) out vec4 _extraDetailsOut;
#endif

void _output(in vec3 normal, in float alphaFactor, in vec2 uv) {
#if defined(USE_ALPHA_DISCARD)
    mat4 colourMatrix = dvd_Matrices[VAR.dvd_baseInstance]._colourMatrix;
    if (getAlbedo(colourMatrix, uv).a * alphaFactor < 1.0f - Z_TEST_SIGMA) {
        discard;
    }
#endif

#if defined(USE_DEFERRED_NORMALS)
    _normalAndVelocityOut.rg = packNormal(normal);
    for (int i = 0; i < 3; ++i) {
        //_extraDetailsOut[i] = getShadowFactor(i);
    }
#endif
}

void outputNoVelocity(in vec2 uv, float alphaFactor) {
    _output(getNormal(uv), alphaFactor, uv);
#if defined(USE_DEFERRED_NORMALS)
    _normalAndVelocityOut.ba = vec2(1.0f);
#endif
}

void outputNoVelocity(in vec2 uv) {
    outputNoVelocity(uv, 1.0f);
}

void outputWithVelocity(in vec2 uv, float alphaFactor) {
    _output(getNormal(uv), alphaFactor, uv);
#if defined(USE_DEFERRED_NORMALS)
    _normalAndVelocityOut.ba = velocityCalc(gl_FragCoord.z, dvd_InvProjectionMatrix, getScreenPositionNormalised());
#endif
}

void outputWithVelocity(in vec2 uv) {
    outputWithVelocity(uv, 1.0f);
}

void outputNoVelocity(in vec2 uv, vec3 normal, float alphaFactor) {
    _output(normal, alphaFactor, uv);
#if defined(USE_DEFERRED_NORMALS)
    _normalAndVelocityOut.ba = vec2(1.0f);
#endif
}

void outputNoVelocity(in vec2 uv, vec3 normal) {
    outputNoVelocity(uv, normal, 1.0f);
}

void outputWithVelocity(in vec2 uv, vec3 normal, float alphaFactor) {
    _output(normal, alphaFactor, uv);
#if defined(USE_DEFERRED_NORMALS)
    _normalAndVelocityOut.ba = velocityCalc(gl_FragCoord.z, dvd_InvProjectionMatrix, getScreenPositionNormalised());
#endif
}

void outputWithVelocity(in vec2 uv, vec3 normal) {
    outputWithVelocity(uv, normal, 1.0f);
}
#endif //_PRE_PASS_FRAG_