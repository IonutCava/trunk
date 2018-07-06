#include "Headers/ParticleColorGenerator.h"

namespace Divide {

void ParticleColorGenerator::generate(vectorImpl<std::future<void>>& packagedTasks,
                                      const U64 deltaTime,
                                      std::shared_ptr<ParticleData> p,
                                      U32 startIndex,
                                      U32 endIndex) {

    const vec4<F32> minStartCol(Util::ToFloatColor(_minStartCol));
    const vec4<F32> maxStartCol(Util::ToFloatColor(_maxStartCol));
    const vec4<F32> minEndCol(Util::ToFloatColor(_minEndCol));
    const vec4<F32> maxEndCol(Util::ToFloatColor(_maxEndCol));

    for (U32 i = startIndex; i < endIndex; ++i) {
        p->_startColor[i].set(Random(minStartCol, maxStartCol));
        p->_endColor[i].set(Random(minEndCol, maxEndCol));
    }

    typedef decltype(std::begin(p->_startColor)) iter_t_start;
    typedef decltype(std::begin(p->_endColor)) iter_t_end;
    for_each_interval<iter_t_start>(std::begin(p->_startColor) + startIndex,
                                    std::begin(p->_startColor) + endIndex,
                                    ParticleData::g_threadPartitionSize,
                                    [&](iter_t_start from, iter_t_start to)
    {
        packagedTasks.push_back(
            std::async(std::launch::async | std::launch::deferred,
                       [from, to, minStartCol, maxStartCol]() {
                           std::for_each(from, to, [&](iter_t_start::value_type& color)
                           {
                               color.set(Random(minStartCol, maxStartCol));
                           });
                       }));
    });

    for_each_interval<iter_t_end>(std::begin(p->_endColor) + startIndex,
                                  std::begin(p->_endColor) + endIndex,
                                  ParticleData::g_threadPartitionSize,
                                  [&](iter_t_end from, iter_t_end to)
    {
        packagedTasks.push_back(
            std::async(std::launch::async | std::launch::deferred,
                       [from, to, minEndCol, maxEndCol]() {
                           std::for_each(from, to, [&](iter_t_end::value_type& color)
                           {
                               color.set(Random(minEndCol, maxEndCol));
                           });
                       }));
    });
}
};