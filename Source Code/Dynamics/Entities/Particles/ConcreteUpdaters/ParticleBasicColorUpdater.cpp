#include "Headers/ParticleBasicColorUpdater.h"
#include <future>

namespace Divide {

void ParticleBasicColorUpdater::update(const U64 deltaTime, std::shared_ptr<ParticleData> p) {
    const U32 endID = p->aliveCount();

   auto parseRange = [&p](U32 start, U32 end) -> void {
        for (U32 i = start; i < end; ++i) {
            p->_color[i].set(Lerp(p->_startColor[i], p->_endColor[i], p->_misc[i].y));
        }
    };

    const U32 half = endID / 2;

    std::future<void> firstHalf =
        std::async(std::launch::async | std::launch::deferred, parseRange, 0, half);

    std::future<void> secondHalf =
        std::async(std::launch::async | std::launch::deferred, parseRange, half, endID);

    firstHalf.get();
    secondHalf.get();
}
};