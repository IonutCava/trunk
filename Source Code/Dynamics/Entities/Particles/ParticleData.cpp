#include "Headers/ParticleData.h"
#include "Core/Headers/Kernel.h"

namespace Divide {

ParticleData::ParticleData(U32 particleCount, U32 optionsMask) {
    generateParticles(particleCount, optionsMask);
}

ParticleData::~ParticleData() { generateParticles(0, _optionsMask); }

void ParticleData::generateParticles(U32 particleCount, U32 optionsMask) {
    _totalCount = particleCount;
    _aliveCount = 0;
    _optionsMask = optionsMask;

    _indices.clear();
    _position.clear();
    _velocity.clear();
    _acceleration.clear();
    _misc.clear();
    _color.clear();
    _startColor.clear();
    _endColor.clear();

    if (_totalCount > 0) {
        if (BitCompare(_optionsMask,
                       to_const_uint(Properties::PROPERTIES_POS))) {
            _position.resize(_totalCount, vec4<F32>(0.0f));
        }
        if (BitCompare(_optionsMask,
                       to_const_uint(Properties::PROPERTIES_VEL))) {
            _velocity.resize(_totalCount, vec4<F32>(0.0f));
        }
        if (BitCompare(_optionsMask,
                       to_const_uint(Properties::PROPERTIES_ACC))) {
            _acceleration.resize(_totalCount, vec4<F32>(0.0f));
        }
        if (BitCompare(_optionsMask,
                       to_const_uint(Properties::PROPERTIES_COLOR))) {
            _color.resize(_totalCount, vec4<F32>(0.0f));
        }
        if (BitCompare(_optionsMask,
                       to_const_uint(Properties::PROPERTIES_COLOR_TRANS))) {
            _startColor.resize(_totalCount, vec4<F32>(0.0f));
            _endColor.resize(_totalCount, vec4<F32>(0.0f));
        }
        _misc.resize(_totalCount, vec4<F32>(0.0f));
    }
}

void ParticleData::kill(U32 index) {
    swapData(index, _aliveCount - 1);
    _aliveCount--;
}

void ParticleData::wake(U32 index) {
    //swapData(index, _aliveCount);
    _aliveCount++;
}

void ParticleData::sort(bool invalidateCache) {
    U32 count = aliveCount();

    if (count == 0) {
        return;
    }

    _indices.resize(count);
    _renderingPositions.resize(count);
    _renderingColors.resize(count);

    for (U32 i = 0; i < count; ++i) {
        std::pair<U32, F32>& idx = _indices[i];
        idx.first = i;
        idx.second = _misc[i].w;
    }
    
    auto sortFunc = 
    [](const std::pair<U32, F32>& indexA, const std::pair<U32, F32>& indexB) {
        return indexA.second > indexB.second;
    };

    std::sort(std::begin(_indices), std::end(_indices), sortFunc);

   auto parsePositions = [count, this](const std::atomic_bool& stopRequested) -> void {
        for (U32 i = 0; i < count; ++i) {
            _renderingPositions[i].set(_position[_indices[i].first]);
        }
    };

    auto parseColors = [count, this](const std::atomic_bool& stopRequested) -> void {
        for (U32 i = 0; i < count; ++i) {
            Util::ToByteColor(_color[_indices[i].first], _renderingColors[i]);
        }
    };
    
    Kernel& kernel = Application::getInstance().getKernel();
    TaskHandle updateTask = kernel.AddTask(DELEGATE_CBK_PARAM<bool>());
    updateTask.addChildTask(kernel.AddTask(parsePositions)._task);
    updateTask.addChildTask(kernel.AddTask(parseColors)._task);
    updateTask.startTask(Task::TaskPriority::HIGH);
    updateTask.wait();

}

void ParticleData::swapData(U32 indexA, U32 indexB) {
    if (BitCompare(_optionsMask, to_const_uint(Properties::PROPERTIES_POS))) {
        _position[indexA].set(_position[indexB]);
    }
    if (BitCompare(_optionsMask, to_const_uint(Properties::PROPERTIES_VEL))) {
        _velocity[indexA].set(_velocity[indexB]);
    }
    if (BitCompare(_optionsMask, to_const_uint(Properties::PROPERTIES_ACC))) {
        _acceleration[indexA].set(_acceleration[indexB]);
    }
    if (BitCompare(_optionsMask, to_const_uint(Properties::PROPERTIES_COLOR))) {
        _color[indexA].set(_color[indexB]);
    }
    if (BitCompare(_optionsMask, to_const_uint(Properties::PROPERTIES_COLOR_TRANS))) {
        _startColor[indexA].set(_startColor[indexB]);
        _endColor[indexA].set(_endColor[indexB]);
    }
    _misc[indexA].set(_misc[indexB]);
}
};