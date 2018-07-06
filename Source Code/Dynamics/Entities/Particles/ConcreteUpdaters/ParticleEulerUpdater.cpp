#include "Headers/ParticleEulerUpdater.h"
#include "Core/Headers/TaskPool.h"

namespace Divide {

void ParticleEulerUpdater::update(const U64 deltaTime, ParticleData& p) {
    F32 const dt = Time::MicrosecondsToSeconds<F32>(deltaTime);
    const vec4<F32> globalA(dt * _globalAcceleration, 0.0f);

    const U32 endID = p.aliveCount();

    auto parseRange = [&p, dt, globalA](const std::atomic_bool& stopRequested, U32 start, U32 end) -> void {
        vectorImplAligned<vec4<F32>>& acceleration = p._acceleration;
        for (U32 i = start; i < end; ++i) {
            vec4<F32>& acc = acceleration[i];
            acc.xyz(acc + globalA);
        }
        vectorImplAligned<vec4<F32>>& velocity = p._velocity;
        for (U32 i = start; i < end; ++i) {
            vec4<F32>& vel = velocity[i];
            vel.xyz(vel + (dt * acceleration[i]));
        }

        vectorImplAligned<vec4<F32>>& position = p._position;
        for (U32 i = start; i < end; ++i) {
            vec4<F32>& pos = position[i];
            pos.xyz(pos + (dt * velocity[i]));
        }
    };

    static const U32 partitionSize = 256;
    U32 crtPartitionSize = std::min(partitionSize, endID);
    U32 partitionCount = endID / crtPartitionSize;
    U32 remainder = endID % crtPartitionSize;
    
    TaskHandle updateTask = CreateTask(DELEGATE_CBK_PARAM<bool>());
    for (U32 i = 0; i < partitionCount; ++i) {
        U32 start = i * crtPartitionSize;
        U32 end = start + crtPartitionSize;
        updateTask.addChildTask(CreateTask(DELEGATE_BIND(parseRange,
                                                      std::placeholders::_1,
                                                      start,
                                                      end))._task)->startTask(Task::TaskPriority::HIGH);
    }
    if (remainder > 0) {
        updateTask.addChildTask(CreateTask(DELEGATE_BIND(parseRange,
                                                      std::placeholders::_1,
                                                      endID - remainder,
                                                      endID))._task)->startTask(Task::TaskPriority::HIGH);
    }
    updateTask.startTask(Task::TaskPriority::HIGH);
    updateTask.wait();
}
};