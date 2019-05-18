#include "stdafx.h"

#include "Headers/ParticleColourGenerator.h"

namespace Divide {

void ParticleColourGenerator::generate(Task& packagedTasksParent,
                                       const U64 deltaTimeUS,
                                       ParticleData& p,
                                       U32 startIndex,
                                       U32 endIndex) {

    const FColour minStartCol(Util::ToFloatColour(_minStartCol));
    const FColour maxStartCol(Util::ToFloatColour(_maxStartCol));
    const FColour minEndCol(Util::ToFloatColour(_minEndCol));
    const FColour maxEndCol(Util::ToFloatColour(_maxEndCol));

    TaskPool& tp = *packagedTasksParent._parentPool;

    typedef decltype(std::begin(p._startColour)) iter_t_start;
    typedef decltype(std::begin(p._endColour)) iter_t_end;
    for_each_interval<iter_t_start>(std::begin(p._startColour) + startIndex,
                                    std::begin(p._startColour) + endIndex,
                                    ParticleData::g_threadPartitionSize,
                                    [&](iter_t_start from, iter_t_start to)
    {
        Start(*CreateTask(tp,
                   &packagedTasksParent,
                   [from, to, minStartCol, maxStartCol](const Task& parentTask) {
                       std::for_each(from, to, [&](iter_t_start::value_type& colour)
                       {
                           colour.set(Random(minStartCol, maxStartCol));
                       });
                   },
                   "ParticleColourGenerator::startColour"));
    });

    for_each_interval<iter_t_end>(std::begin(p._endColour) + startIndex,
                                  std::begin(p._endColour) + endIndex,
                                  ParticleData::g_threadPartitionSize,
                                  [&](iter_t_end from, iter_t_end to)
    {
        Start(*CreateTask(tp,
                   &packagedTasksParent,
                   [from, to, minEndCol, maxEndCol](const Task& parentTask) {
                       std::for_each(from, to, [&](iter_t_end::value_type& colour)
                       {
                           colour.set(Random(minEndCol, maxEndCol));
                       });
                   },
                   "ParticleColourGenerator::endColour"));
    });
}
};