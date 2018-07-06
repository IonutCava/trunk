#include "Headers/ParticleEulerUpdater.h"
#include "Core/Headers/Kernel.h"

#include <future>

namespace Divide {

void ParticleEulerUpdater::update(const U64 deltaTime, ParticleData& p) {
    F32 const dt = Time::MicrosecondsToSeconds<F32>(deltaTime);
    const vec4<F32> globalA(dt * _globalAcceleration, 0.0f);

    const U32 endID = p.aliveCount();

    auto parseRange = [&p, dt, globalA](const std::atomic_bool& stopRequested, U32 start, U32 end) -> void {
        vectorImpl<vec4<F32>>& acceleration = p._acceleration;
        for (U32 i = start; i < end; ++i) {
            vec4<F32>& acc = acceleration[i];
            acc.xyz(acc + globalA);
        }
        vectorImpl<vec4<F32>>& velocity = p._velocity;
        for (U32 i = start; i < end; ++i) {
            vec4<F32>& vel = velocity[i];
            vel.xyz(vel + (dt * acceleration[i]));
        }

        vectorImpl<vec4<F32>>& position = p._position;
        for (U32 i = start; i < end; ++i) {
            vec4<F32>& pos = position[i];
            pos.xyz(pos + (dt * velocity[i]));
        }
    };

    static const U32 partitionSize = 256;
    U32 crtPartitionSize = std::min(partitionSize, endID);
    U32 partitionCount = endID / crtPartitionSize;
    U32 remainder = endID % crtPartitionSize;
    
    const U32 half = endID / 2u;
    Kernel& kernel = Application::getInstance().getKernel();
    TaskHandle updateTask = kernel.AddTask(DELEGATE_CBK_PARAM<bool>());
    for (U32 i = 0; i < partitionCount; ++i) {
        U32 start = i * crtPartitionSize;
        U32 end = start + crtPartitionSize;
        updateTask.addChildTask(kernel.AddTask(DELEGATE_BIND(parseRange, std::placeholders::_1, start, end))._task);
    }
    if (remainder > 0) {
        updateTask.addChildTask(kernel.AddTask(DELEGATE_BIND(parseRange, std::placeholders::_1, endID - remainder, endID))._task);
    }
    updateTask.startTask(Task::TaskPriority::HIGH);
    updateTask.wait();
}
};