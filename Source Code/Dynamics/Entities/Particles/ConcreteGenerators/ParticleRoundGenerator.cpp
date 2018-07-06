#include "stdafx.h"

#include "Headers/ParticleRoundGenerator.h"

namespace Divide {

void ParticleRoundGenerator::generate(TaskHandle& packagedTasksParent,
                                      const U64 deltaTimeUS,
                                      ParticleData& p,
                                      U32 startIndex,
                                      U32 endIndex) {

    vec3<F32> center(_center + _sourcePosition);
    for (U32 i = startIndex; i < endIndex; i++) {
        F32 ang = Random(0.0f, to_F32(M_2PI));
        p._position[i].xyz(center + vec3<F32>(_radX * std::sin(ang),
                                              _radY * std::cos(ang), 
                                              0.0f));
    }
}
};