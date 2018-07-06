#include "Headers/ParticleVelocityGenerator.h"

namespace Divide {

void ParticleVelocityGenerator::generate(TaskHandle& packagedTasksParent, 
                                         const U64 deltaTime,
                                         ParticleData& p,
                                         U32 startIndex,
                                         U32 endIndex) {
    vec3<F32> min = _sourceOrientation * _minStartVel.xyz();
    vec3<F32> max = _sourceOrientation * _maxStartVel.xyz();
    
    TaskPool& tp = packagedTasksParent._task->getOwningPool();

    typedef decltype(std::begin(p._velocity)) iter_t;
    for_each_interval<iter_t>(std::begin(p._velocity) + startIndex,
                              std::begin(p._velocity) + endIndex,
                              ParticleData::g_threadPartitionSize,
                              [&](iter_t from, iter_t to)
    {
        packagedTasksParent.addChildTask(CreateTask(tp,
            [from, to, min, max](const std::atomic_bool& stopRequested) mutable
            {
                std::for_each(from, to, [&](iter_t::value_type& velocity)
                {
                    velocity.set(Random(min, max));
                });
            })._task)->startTask(Task::TaskPriority::HIGH);
        });
}
};