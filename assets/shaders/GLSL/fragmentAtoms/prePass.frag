#ifndef _PRE_PASS_FRAG_
#define _PRE_PASS_FRAG_

#if !defined(PRE_PASS)
#define PRE_PASS
#endif

#include "materialData.frag"
#include "velocityCalc.frag"
#include "shadowUtils.frag"

layout(location = TARGET_NORMALS_AND_VELOCITY) out vec4 _normalAndVelocityOut;
//r,g,b = CSM shadow factor for light ids 0, 1 & 2; a - specular/roughness
layout(location = TARGET_EXTRA) out vec4 _extraDetailsOut;

void _output(in vec3 normal, in float alphaFactor, in vec2 uv) {
#if defined(USE_ALPHA_DISCARD)
    mat4 colourMatrix = dvd_Matrices[VAR.dvd_baseInstance]._colourMatrix;
    if (getAlbedo(colourMatrix, uv).a * alphaFactor < 1.0f - Z_TEST_SIGMA) {
        discard;
    }
#endif

    _normalAndVelocityOut.rg = packNormal(normalize(normal));
    for (int i = 0; i < 3; ++i) {
        //_extraDetailsOut[i] = getShadowFactor(i);
    }
}

void outputNoVelocity(in vec2 uv, float alphaFactor) {
    _output(getNormal(uv), alphaFactor, uv);
    _normalAndVelocityOut.ba = vec2(1.0f);
}

void outputNoVelocity(in vec2 uv) {
    _output(getNormal(uv), 1.0f, uv);
    _normalAndVelocityOut.ba = vec2(1.0f);
}

void outputWithVelocity(in vec2 uv, float alphaFactor) {
    _output(getNormal(uv), alphaFactor, uv);
    _normalAndVelocityOut.ba = velocityCalc(dvd_InvProjectionMatrix, getScreenPositionNormalised());
}

void outputWithVelocity(in vec2 uv) {
    _output(getNormal(uv), 1.0f, uv);
    _normalAndVelocityOut.ba = velocityCalc(dvd_InvProjectionMatrix, getScreenPositionNormalised());
}

void outputNoVelocity(in vec2 uv, vec3 normal, float alphaFactor) {
    _output(normal, alphaFactor, uv);
    _normalAndVelocityOut.ba = vec2(1.0f);
}

void outputNoVelocity(in vec2 uv, vec3 normal) {
    _output(normal, 1.0f, uv);
    _normalAndVelocityOut.ba = vec2(1.0f);
}

void outputWithVelocity(in vec2 uv, vec3 normal, float alphaFactor) {
    _output(normal, alphaFactor, uv);
    _normalAndVelocityOut.ba = velocityCalc(dvd_InvProjectionMatrix, getScreenPositionNormalised());
}

void outputWithVelocity(in vec2 uv, vec3 normal) {
    _output(normal, 1.0f, uv);
    _normalAndVelocityOut.ba = velocityCalc(dvd_InvProjectionMatrix, getScreenPositionNormalised());
}
#endif //_PRE_PASS_FRAG_