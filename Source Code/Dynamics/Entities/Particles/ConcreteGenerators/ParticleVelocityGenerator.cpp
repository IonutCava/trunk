#include "stdafx.h"

#include "Headers/ParticleVelocityGenerator.h"

namespace Divide {

void ParticleVelocityGenerator::generate(Task& packagedTasksParent, 
                                         const U64 deltaTimeUS,
                                         ParticleData& p,
                                         U32 startIndex,
                                         U32 endIndex) {
    vec3<F32> min = _sourceOrientation * _minStartVel.xyz();
    vec3<F32> max = _sourceOrientation * _maxStartVel.xyz();
    
    TaskPool& tp = *packagedTasksParent._parentPool;

    typedef decltype(std::begin(p._velocity)) iter_t;
    for_each_interval<iter_t>(std::begin(p._velocity) + startIndex,
                              std::begin(p._velocity) + endIndex,
                              ParticleData::g_threadPartitionSize,
                              [&](iter_t from, iter_t to)
    {
        Start(*CreateTask(tp,
            &packagedTasksParent,
            [from, to, min, max](const Task& parentTask) mutable
            {
                std::for_each(from, to, [&](iter_t::value_type& velocity)
                {
                    velocity.set(Random(min, max));
                });
            },
            "ParticleVelocityGenerator"));
        });
}
};