#ifndef _PRE_PASS_FRAG_
#define _PRE_PASS_FRAG_

#include "lightInput.cmn"
#include "materialData.frag"
#include "velocityCalc.frag"
#include "shadowMapping.frag"

layout(location = TARGET_NORMALS_AND_VELOCITY) out vec4 _normalAndVelocityOut;
layout(location = TARGET_EXTRA) out vec4 _lightDetailsOut;

void _writeCSMSplit() {
    _lightDetailsOut = vec4(getCSMSlice(0), 0, 0, 0);
}

void _output(vec3 normal, float alphaFactor) {
    updateTexCoord();

#if defined(USE_ALPHA_DISCARD)
    mat4 colourMatrix = dvd_Matrices[VAR.dvd_baseInstance]._colourMatrix;
    if (getAlbedo(colourMatrix).a * alphaFactor < 1.0f - Z_TEST_SIGMA) {
        discard;
    }
#endif

    _normalAndVelocityOut.rg = packNormal(normalize(normal));
    _writeCSMSplit();
}

void outputNoVelocity(float alphaFactor) {
    _output(getNormal(), alphaFactor);
    _normalAndVelocityOut.ba = vec2(1.0f);
}

void outputNoVelocity() {
    _output(getNormal(), 1.0f);
    _normalAndVelocityOut.ba = vec2(1.0f);
}

void outputWithVelocity(float alphaFactor) {
    _output(getNormal(), alphaFactor);
    _normalAndVelocityOut.ba = velocityCalc(dvd_InvProjectionMatrix, getScreenPositionNormalised());
}

void outputWithVelocity() {
    _output(getNormal(), 1.0f);
    _normalAndVelocityOut.ba = velocityCalc(dvd_InvProjectionMatrix, getScreenPositionNormalised());
}

void outputNoVelocity(vec3 normal, float alphaFactor) {
    _output(normal, alphaFactor);
    _normalAndVelocityOut.ba = vec2(1.0f);
}

void outputNoVelocity(vec3 normal) {
    _output(normal, 1.0f);
    _normalAndVelocityOut.ba = vec2(1.0f);
}

void outputWithVelocity(vec3 normal, float alphaFactor) {
    _output(normal, alphaFactor);
    _normalAndVelocityOut.ba = velocityCalc(dvd_InvProjectionMatrix, getScreenPositionNormalised());
}

void outputWithVelocity(vec3 normal) {
    _output(normal, 1.0f);
    _normalAndVelocityOut.ba = velocityCalc(dvd_InvProjectionMatrix, getScreenPositionNormalised());
}
#endif //_PRE_PASS_FRAG_