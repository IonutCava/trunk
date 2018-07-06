#include "Headers/ParticleVelocityGenerator.h"

namespace Divide {

void ParticleVelocityGenerator::generate(vectorImpl<std::future<void>>& packagedTasks, 
                                         const U64 deltaTime,
                                         std::shared_ptr<ParticleData> p,
                                         U32 startIndex,
                                         U32 endIndex) {
    vec3<F32> min(_minStartVel);
    vec3<F32> max(_maxStartVel);
    typedef decltype(std::begin(p->_velocity)) iter_t;
    for_each_interval<iter_t>(std::begin(p->_velocity) + startIndex,
                              std::begin(p->_velocity) + endIndex,
                              ParticleData::g_threadPartitionSize,
                              [&](iter_t from, iter_t to)
    {
        packagedTasks.push_back(
            std::async(std::launch::async | std::launch::deferred,
                       [from, to, min, max]() {
                           std::for_each(from, to, [&](iter_t::value_type& velocity)
                           {
                               velocity.set(Random(min, max));
                           });
                      }));
    });
}
};