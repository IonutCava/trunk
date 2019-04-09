#ifndef _PRE_PASS_FRAG_
#define _PRE_PASS_FRAG_

#include "materialData.frag"
#include "velocityCalc.frag"

layout(location = 1) out vec4 _normalAndVelocityOut;

void _output(vec3 normal, float alphaFactor) {
    updateTexCoord();

#if defined(USE_ALPHA_DISCARD)
    mat4 colourMatrix = dvd_Matrices[VAR.dvd_baseInstance]._colourMatrix;
    if (getAlbedo(colourMatrix).a * alphaFactor < 1.0f - Z_TEST_SIGMA) {
        discard;
    }
#endif

    _normalAndVelocityOut.rg = packNormal(normalize(normal));
}

void outputNoVelocity(float alphaFactor = 1.0f) {
    _output(getNormal(), alphaFactor);
    _normalAndVelocityOut.ba = vec2(1.0f);
}

void outputWithVelocity(float alphaFactor = 1.0f) {
    _output(getNormal(), alphaFactor);
    _normalAndVelocityOut.ba = velocityCalc(dvd_InvProjectionMatrix, getScreenPositionNormalised());
}

void outputNoVelocity(vec3 normal, float alphaFactor = 1.0f) {
    _output(normal, alphaFactor);
    _normalAndVelocityOut.ba = vec2(1.0f);
}

void outputWithVelocity(vec3 normal, float alphaFactor = 1.0f) {
    _output(normal, alphaFactor);
    _normalAndVelocityOut.ba = velocityCalc(dvd_InvProjectionMatrix, getScreenPositionNormalised());
}
#endif //_PRE_PASS_FRAG_