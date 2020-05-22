#include "stdafx.h"

#include "Headers/SceneEnvironmentProbePool.h"
#include "Scenes/Headers/Scene.h"

#include "ECS/Components/Headers/EnvironmentProbeComponent.h"

namespace Divide {

SceneEnvironmentProbePool::SceneEnvironmentProbePool(Scene& parentScene)
    : SceneComponent(parentScene)
{
}

const EnvironmentProbeList& SceneEnvironmentProbePool::sortAndGetLocked(const vec3<F32>& position) {
    eastl::sort(eastl::begin(_envProbes),
                eastl::end(_envProbes),
                [&position](const auto& a, const auto& b) -> bool {
                    return a->distanceSqTo(position) < b->distanceSqTo(position);
                });

    return _envProbes;
}

void SceneEnvironmentProbePool::lockProbeList() {
    _probeLock.lock();
}

void SceneEnvironmentProbePool::unlockProbeList() {
    _probeLock.unlock();
}

const EnvironmentProbeList& SceneEnvironmentProbePool::getLocked() {
    return _envProbes;
}

EnvironmentProbeComponent* SceneEnvironmentProbePool::registerProbe(EnvironmentProbeComponent* probe) {
    UniqueLock<SharedMutex> w_lock(_probeLock);
    _envProbes.emplace_back(probe);
    return probe;
}

void SceneEnvironmentProbePool::unregisterProbe(EnvironmentProbeComponent* probe) {
    if (probe != nullptr) {
        UniqueLock<SharedMutex> w_lock(_probeLock);
        I64 probeGUID = probe->getGUID();
        _envProbes.erase(eastl::remove_if(eastl::begin(_envProbes), eastl::end(_envProbes),
                                       [&probeGUID](const auto& probe)
                                            -> bool { return probe->getGUID() == probeGUID; }),
                         eastl::end(_envProbes));
    }
}

}; //namespace Divide