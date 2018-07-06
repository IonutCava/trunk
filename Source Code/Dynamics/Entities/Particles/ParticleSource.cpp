#include "Headers/ParticleSource.h"

namespace Divide {

ParticleSource::ParticleSource() : ParticleSource(0)
{
}

ParticleSource::ParticleSource(F32 emitRate) : _emitRate(emitRate)
{
}

ParticleSource::~ParticleSource()
{
}

void ParticleSource::emit(const U64 deltaTime, std::shared_ptr<ParticleData> p) {
    ParticleData& data = *p;

    const F32 dt = Time::MicrosecondsToSeconds<F32>(deltaTime);
    const U32 maxNewParticles = to_uint(dt * _emitRate);
    const U32 startID = data.aliveCount();
    const U32 endID = std::min(startID + maxNewParticles, data.totalCount() - 1);

    TaskHandle generateTask = CreateTask(DELEGATE_CBK_PARAM<const Task&>());
    for (std::shared_ptr<ParticleGenerator>& gen : _particleGenerators) {
        gen->generate(generateTask, deltaTime, data, startID, endID);
    }
    generateTask.startTask(Task::TaskPriority::HIGH);
    generateTask.wait();

    for (U32 i = startID; i < endID; ++i) {
        p->wake(i);
    }
}
};