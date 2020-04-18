#include "stdafx.h"

#include "Headers/SceneEnvironmentProbePool.h"
#include "Scenes/Headers/Scene.h"

namespace Divide {

SceneEnvironmentProbePool::SceneEnvironmentProbePool(Scene& parentScene)
    : SceneComponent(parentScene),
      _isSorted(false)
{
}

SceneEnvironmentProbePool::~SceneEnvironmentProbePool()
{
}

const EnvironmentProbeList& SceneEnvironmentProbePool::getNearestSorted(const vec3<F32>& position) {
    if (!_isSorted) {
        _sortedProbes.resize(0);

        _sortedProbes.insert(eastl::cend(_sortedProbes), eastl::cbegin(_envProbes), eastl::cend(_envProbes));

        auto sortFunc = [&position](const EnvironmentProbe_ptr& a, const EnvironmentProbe_ptr& b) -> bool {
            return a->distanceSqTo(position) < b->distanceSqTo(position);
        };

        eastl::sort(eastl::begin(_sortedProbes), eastl::end(_sortedProbes), sortFunc);
        U32 lowerLimit = std::min(Config::MAX_REFLECTIVE_PROBES_PER_PASS, to_U32(_sortedProbes.size()));
        _sortedProbes.erase(eastl::begin(_sortedProbes) + lowerLimit, eastl::end(_sortedProbes));
        _isSorted = true;
    }

    return _sortedProbes;
}


EnvironmentProbe_wptr SceneEnvironmentProbePool::addInfiniteProbe(const vec3<F32>& position) {
    EnvironmentProbe_ptr probe = std::make_shared<EnvironmentProbe>(parentScene(), EnvironmentProbe::ProbeType::TYPE_INFINITE);
    probe->setBounds(position, 1000.0f);
    _envProbes.push_back(probe);
    _isSorted = false;
    return probe;
}

EnvironmentProbe_wptr SceneEnvironmentProbePool::addLocalProbe(const vec3<F32>& bbMin,
                                                               const vec3<F32>& bbMax) {
    EnvironmentProbe_ptr probe = std::make_shared<EnvironmentProbe>(parentScene(), EnvironmentProbe::ProbeType::TYPE_LOCAL);
    probe->setBounds(bbMin, bbMax);
    _envProbes.push_back(probe);
    _isSorted = false;
    return probe;
}

void SceneEnvironmentProbePool::removeProbe(EnvironmentProbe_wptr probe) {
    if (!probe.expired()) {
        EnvironmentProbe_ptr probePtr = probe.lock();
        I64 probeGUID = probePtr->getGUID();
        _envProbes.erase(eastl::remove_if(eastl::begin(_envProbes), eastl::end(_envProbes),
                                       [&probeGUID](const EnvironmentProbe_ptr& probe)
                                            -> bool { return probe->getGUID() == probeGUID; }),
                         eastl::end(_envProbes));
        probePtr.reset();
        _isSorted = false;
    }
}

void SceneEnvironmentProbePool::debugDraw(GFX::CommandBuffer& bufferInOut) {
    for (const EnvironmentProbe_ptr& probe : _envProbes) {
        probe->debugDraw(bufferInOut);
    }
}

}; //namespace Divide