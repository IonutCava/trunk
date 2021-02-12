#ifndef _PRE_PASS_FRAG_
#define _PRE_PASS_FRAG_

#if !defined(PRE_PASS)
#   define PRE_PASS
#endif //!PRE_PASS

#if !defined(HAS_TRANSPARENCY)
#if defined(USE_ALPHA_DISCARD) || defined(USE_ALBEDO_ALPHA) || defined(USE_OPACITY_MAP)
#   define HAS_TRANSPARENCY
#endif //USE_ALPHA_DISCARD || USE_ALBEDO_ALPHA || USE_OPACITY_MAP
#endif //!HAS_TRANSPARENCY

// This will also do our check for us
#include "velocityCalc.frag"

#if defined(HAS_VELOCITY)
layout(location = TARGET_VELOCITY) out vec2 _velocityOut;
#endif //HAS_VELOCITY

#if defined(USE_ALPHA_DISCARD)
#include "materialData.frag"
#endif //USE_ALPHA_DISCARD

void writeGBuffer() {
#if defined(HAS_VELOCITY)
    _velocityOut = velocityCalc();
#endif //HAS_VELOCITY
}

void writeGBuffer(in float albedoAlpha)
{
#if defined(USE_ALPHA_DISCARD)
    if (albedoAlpha < INV_Z_TEST_SIGMA) {
        discard;
    }
#endif //USE_ALPHA_DISCARD

    writeGBuffer();
}

#endif //_PRE_PASS_FRAG_
