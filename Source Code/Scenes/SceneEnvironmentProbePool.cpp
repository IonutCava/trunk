#include "Headers/SceneEnvironmentProbePool.h"
#include "Scenes/Headers/Scene.h"

namespace Divide {

SceneEnvironmentProbePool::SceneEnvironmentProbePool(Scene& parentScene)
    : SceneComponent(parentScene)
{
}

SceneEnvironmentProbePool::~SceneEnvironmentProbePool()
{
    MemoryManager::DELETE_VECTOR(_envProbes);
}

const vectorImpl<EnvironmentProbe*>& SceneEnvironmentProbePool::getNearestSorted() {
    _sortedProbes.resize(0);
    const vec3<F32>& camPosition = _parentScene.renderState().getCamera().getEye();

    _sortedProbes.insert(std::cend(_sortedProbes), std::cbegin(_envProbes), std::cend(_envProbes));

    auto sortFunc = [&camPosition](EnvironmentProbe* a, EnvironmentProbe* b) -> bool {
        return a->distanceSqTo(camPosition) < b->distanceSqTo(camPosition);
    };

    std::sort(std::begin(_sortedProbes), std::end(_sortedProbes), sortFunc);
    U32 lowerLimit = std::min(Config::MAX_REFLECTIVE_PROBES_PER_PASS, to_uint(_sortedProbes.size()));
    _sortedProbes.erase(std::begin(_sortedProbes) + lowerLimit, std::end(_sortedProbes));

    return _sortedProbes;
}


void SceneEnvironmentProbePool::addInfiniteProbe(const vec3<F32>& position) {
    EnvironmentProbe* probe = MemoryManager_NEW EnvironmentProbe(EnvironmentProbe::ProbeType::TYPE_INFINITE);
    probe->setBounds(position, 1000.0f);
    _envProbes.push_back(probe);
}

void SceneEnvironmentProbePool::addLocalProbe(const vec3<F32>& bbMin,
                                              const vec3<F32>& bbMax) {
    EnvironmentProbe* probe = MemoryManager_NEW EnvironmentProbe(EnvironmentProbe::ProbeType::TYPE_LOCAL);
    probe->setBounds(bbMin, bbMax);
    _envProbes.push_back(probe);
}

void SceneEnvironmentProbePool::debugDraw() {
    for (EnvironmentProbe* probe : _envProbes) {
        probe->debugDraw();
    }
}

}; //namespace Divide