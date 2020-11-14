#include "stdafx.h"

#include "Headers/ParticleColourGenerator.h"

namespace Divide {

void ParticleColourGenerator::generate(Task& packagedTasksParent,
                                       const U64 deltaTimeUS,
                                       ParticleData& p,
                                       U32 startIndex,
                                       U32 endIndex) {

    TaskPool& tp = *packagedTasksParent._parentPool;

    using iter_t_start = decltype(std::begin(p._startColour));
    for_each_interval<iter_t_start>(std::begin(p._startColour) + startIndex,
                                    std::begin(p._startColour) + endIndex,
                                    ParticleData::g_threadPartitionSize,
                                    [&](iter_t_start from, iter_t_start to)
    {
        Start(*CreateTask(tp,
                   &packagedTasksParent,
                   [this, from, to](const Task&) {
                       std::for_each(from, to, [&](FColour4& colour)
                       {
                           colour.set(Random(_minStartCol, _maxStartCol));
                       });
                   }));
    });

    using iter_t_end = decltype(std::begin(p._endColour));
    for_each_interval<iter_t_end>(std::begin(p._endColour) + startIndex,
                                  std::begin(p._endColour) + endIndex,
                                  ParticleData::g_threadPartitionSize,
                                  [&](iter_t_end from, iter_t_end to)
    {
        Start(*CreateTask(tp,
                   &packagedTasksParent,
                   [this, from, to](const Task&) {
                       std::for_each(from, to, [&](FColour4& colour)
                       {
                           colour.set(Random(_minEndCol, _maxEndCol));
                       });
                   }));
    });
}
}