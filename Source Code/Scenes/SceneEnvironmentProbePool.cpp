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

const EnvironmentProbeList& SceneEnvironmentProbePool::getNearestSorted() {
    if (!_isSorted) {
        _sortedProbes.resize(0);
        const vec3<F32>& camPosition = _parentScene.renderState().getCamera().getEye();

        _sortedProbes.insert(std::cend(_sortedProbes), std::cbegin(_envProbes), std::cend(_envProbes));

        auto sortFunc = [&camPosition](const EnvironmentProbe_ptr& a, const EnvironmentProbe_ptr& b) -> bool {
            return a->distanceSqTo(camPosition) < b->distanceSqTo(camPosition);
        };

        std::sort(std::begin(_sortedProbes), std::end(_sortedProbes), sortFunc);
        U32 lowerLimit = std::min(Config::MAX_REFLECTIVE_PROBES_PER_PASS, to_uint(_sortedProbes.size()));
        _sortedProbes.erase(std::begin(_sortedProbes) + lowerLimit, std::end(_sortedProbes));
        _isSorted = true;
    }

    return _sortedProbes;
}


EnvironmentProbe_wptr SceneEnvironmentProbePool::addInfiniteProbe(const vec3<F32>& position) {
    EnvironmentProbe_ptr probe = std::make_shared<EnvironmentProbe>(EnvironmentProbe::ProbeType::TYPE_INFINITE);
    probe->setBounds(position, 1000.0f);
    _envProbes.push_back(probe);
    _isSorted = false;
    return probe;
}

EnvironmentProbe_wptr SceneEnvironmentProbePool::addLocalProbe(const vec3<F32>& bbMin,
                                                               const vec3<F32>& bbMax) {
    EnvironmentProbe_ptr probe = std::make_shared<EnvironmentProbe>(EnvironmentProbe::ProbeType::TYPE_LOCAL);
    probe->setBounds(bbMin, bbMax);
    _envProbes.push_back(probe);
    _isSorted = false;
    return probe;
}

void SceneEnvironmentProbePool::removeProbe(EnvironmentProbe_wptr probe) {
    if (!probe.expired()) {
        EnvironmentProbe_ptr probePtr = probe.lock();
        I64 probeGUID = probePtr->getGUID();
        _envProbes.erase(std::remove_if(std::begin(_envProbes), std::end(_envProbes),
                                       [&probeGUID](const EnvironmentProbe_ptr& probe)
                                            -> bool { return probe->getGUID() == probeGUID; }),
                         std::end(_envProbes));
        probePtr.reset();
        _isSorted = false;
    }
}

void SceneEnvironmentProbePool::debugDraw(RenderSubPassCmds& subPassesInOut) {
    for (const EnvironmentProbe_ptr& probe : _envProbes) {
        probe->debugDraw(subPassesInOut);
    }
}

}; //namespace Divide