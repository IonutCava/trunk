#include "Headers/ParticleSource.h"

namespace Divide {

ParticleSource::ParticleSource() : _emitRate(0) {}

ParticleSource::ParticleSource(F32 emitRate) : _emitRate(emitRate) {}

ParticleSource::~ParticleSource() {}

void ParticleSource::emit(const U64 deltaTime, std::shared_ptr<ParticleData> p) {
    const F32 dt = Time::MicrosecondsToSeconds<F32>(deltaTime);
    const U32 maxNewParticles = to_uint(dt * _emitRate);
    const U32 startID = p->aliveCount();
    const U32 endID = std::min(startID + maxNewParticles, p->totalCount() - 1);

    for (std::shared_ptr<ParticleGenerator>& gen : _particleGenerators) {
        gen->generate(deltaTime, p, startID, endID);
    }

    for (U32 i = startID; i < endID; ++i) {
        p->wake(i);
    }
}
};