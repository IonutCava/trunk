#include "Headers/ParticleBoxGenerator.h"
#include <future>

namespace Divide {

void ParticleBoxGenerator::generate(vectorImpl<std::future<void>>& packagedTasks, 
                                    const U64 deltaTime,
                                    std::shared_ptr<ParticleData> p,
                                    U32 startIndex,
                                    U32 endIndex) {
    vec3<F32> min(_posMin);
    vec3<F32> max(_posMax);

    typedef decltype(std::begin(p->_position)) iter_t;
    for_each_interval<iter_t>(std::begin(p->_position) + startIndex,
                              std::begin(p->_position) + endIndex,
                              ParticleData::g_threadPartitionSize,
                              [&](iter_t from, iter_t to)
    {
        packagedTasks.push_back(
            std::async(std::launch::async | std::launch::deferred,
                      [from, to, &min, &max]() {
                          std::for_each(from, to, [&](iter_t::value_type& position)
                          {
                              position.xyz(Random(min, max));
                          });
                      }));
    });
}

};