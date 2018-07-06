#include "Headers/ParticleColourGenerator.h"

namespace Divide {

void ParticleColourGenerator::generate(TaskHandle& packagedTasksParent,
                                      const U64 deltaTime,
                                      ParticleData& p,
                                      U32 startIndex,
                                      U32 endIndex) {

    const vec4<F32> minStartCol(Util::ToFloatColour(_minStartCol));
    const vec4<F32> maxStartCol(Util::ToFloatColour(_maxStartCol));
    const vec4<F32> minEndCol(Util::ToFloatColour(_minEndCol));
    const vec4<F32> maxEndCol(Util::ToFloatColour(_maxEndCol));

    typedef decltype(std::begin(p._startColour)) iter_t_start;
    typedef decltype(std::begin(p._endColour)) iter_t_end;
    for_each_interval<iter_t_start>(std::begin(p._startColour) + startIndex,
                                    std::begin(p._startColour) + endIndex,
                                    ParticleData::g_threadPartitionSize,
                                    [&](iter_t_start from, iter_t_start to)
    {
        packagedTasksParent.addChildTask(CreateTask(
            [from, to, minStartCol, maxStartCol](const std::atomic_bool& stopRequested) {
                std::for_each(from, to, [&](iter_t_start::value_type& colour)
                {
                    colour.set(Random(minStartCol, maxStartCol));
                });
            })._task)->startTask(Task::TaskPriority::HIGH);
    });

    for_each_interval<iter_t_end>(std::begin(p._endColour) + startIndex,
                                  std::begin(p._endColour) + endIndex,
                                  ParticleData::g_threadPartitionSize,
                                  [&](iter_t_end from, iter_t_end to)
    {
        packagedTasksParent.addChildTask(CreateTask(
            [from, to, minEndCol, maxEndCol](const std::atomic_bool& stopRequested) {
                std::for_each(from, to, [&](iter_t_end::value_type& colour)
                {
                    colour.set(Random(minEndCol, maxEndCol));
                });
            })._task)->startTask(Task::TaskPriority::HIGH);
    });
}
};