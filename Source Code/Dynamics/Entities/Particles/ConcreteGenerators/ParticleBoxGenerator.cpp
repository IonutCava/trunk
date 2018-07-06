#include "Headers/ParticleBoxGenerator.h"

namespace Divide {

void ParticleBoxGenerator::generate(TaskHandle& packagedTasksParent,
                                    const U64 deltaTime,
                                    ParticleData& p,
                                    U32 startIndex,
                                    U32 endIndex) {
    vec3<F32> min(_posMin + _sourcePosition);
    vec3<F32> max(_posMax + _sourcePosition);

    typedef decltype(std::begin(p._position)) iter_t;
    for_each_interval<iter_t>(std::begin(p._position) + startIndex,
                              std::begin(p._position) + endIndex,
                              ParticleData::g_threadPartitionSize,
                              [&](iter_t from, iter_t to)
    {
        packagedTasksParent.addChildTask(CreateTask(
            [from, to, min, max](const std::atomic_bool& stopRequested) mutable
            {
                std::for_each(from, to, [min, max](iter_t::value_type& position)
                {
                    position.xyz(Random(min, max));
                });
            })._task)->startTask(Task::TaskPriority::HIGH);
    });
}

};