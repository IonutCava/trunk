#include "stdafx.h"

#include "Headers/SceneEnvironmentProbePool.h"
#include "Scenes/Headers/Scene.h"

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

EnvironmentProbe* SceneEnvironmentProbePool::addInfiniteProbe(const vec3<F32>& position) {
    UniqueLock<SharedMutex> w_lock(_probeLock);
    auto& probe = _envProbes.emplace_back(std::make_unique<EnvironmentProbe>(parentScene(), EnvironmentProbe::ProbeType::TYPE_INFINITE));
    probe->setBounds(position, 1000.0f);
    return probe.get();
}

EnvironmentProbe* SceneEnvironmentProbePool::addLocalProbe(const vec3<F32>& bbMin, const vec3<F32>& bbMax) {
    UniqueLock<SharedMutex> w_lock(_probeLock);
    auto& probe = _envProbes.emplace_back(std::make_unique<EnvironmentProbe>(parentScene(), EnvironmentProbe::ProbeType::TYPE_LOCAL));
    probe->setBounds(bbMin, bbMax);
    return probe.get();
}

void SceneEnvironmentProbePool::removeProbe(EnvironmentProbe*& probe) {
    if (probe != nullptr) {
        UniqueLock<SharedMutex> w_lock(_probeLock);
        I64 probeGUID = probe->getGUID();
        _envProbes.erase(eastl::remove_if(eastl::begin(_envProbes), eastl::end(_envProbes),
                                       [&probeGUID](const auto& probe)
                                            -> bool { return probe->getGUID() == probeGUID; }),
                         eastl::end(_envProbes));
        probe = nullptr;
    }
}

void SceneEnvironmentProbePool::debugDraw(GFX::CommandBuffer& bufferInOut) {
    SharedLock<SharedMutex> w_lock(_probeLock);
    for (const auto& probe : _envProbes) {
        probe->debugDraw(bufferInOut);
    }
}

}; //namespace Divide