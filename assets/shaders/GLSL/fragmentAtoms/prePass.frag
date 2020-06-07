#ifndef _PRE_PASS_FRAG_
#define _PRE_PASS_FRAG_

#if !defined(PRE_PASS)
#define PRE_PASS
#endif

#include "materialData.frag"

#if defined(USE_DEFERRED_NORMALS)

layout(location = TARGET_NORMALS_AND_VELOCITY) out vec4 _normalAndVelocityOut;
//r,g = TBN view direction,  b - ssao, a - extra flags
layout(location = TARGET_EXTRA) out vec4 _extraDetailsOut;

#include "velocityCalc.frag"
#endif

void writeOutput(in NodeData nodeData,
                 vec2 uv,
                 vec3 normal,
                 vec3 tbnViewDirection,
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
    _extraDetailsOut.rg = packNormal(normalize(tbnViewDirection));
    _extraDetailsOut.ba = vec2(1.0f, extraFlag);
#endif
}
void writeOutput(in NodeData nodeData,
                vec2 uv,
                vec3 normal,
                vec3 tbnViewDirection,
                float alphaFactor)
{
    writeOutput(nodeData, uv, normal, tbnViewDirection, alphaFactor, 0.0f);
}

void writeOutput(in NodeData nodeData,
                 vec2 uv,
                 vec3 normal,
                 vec3 tbnViewDirection) 
{
    writeOutput(nodeData, uv, normal, tbnViewDirection, 1.0f, 0.0f);
}
#endif //_PRE_PASS_FRAG_