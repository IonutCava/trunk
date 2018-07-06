#include "Headers/ParticleData.h"

namespace Divide {

namespace {

struct IndexComp {
    IndexComp(ParticleData& refData) : _refData(refData) {}

    inline bool operator()(const U32& indexA, const U32& indexB) {
        F32 distA = _refData._misc[indexA].w;
        F32 distB = _refData._misc[indexB].w;
        return distA < distB;
    }

    ParticleData& _refData;
};
};

ParticleData::ParticleData(U32 particleCount, U32 optionsMask) {
    generateParticles(particleCount, optionsMask);
}

ParticleData::~ParticleData() { generateParticles(0, _optionsMask); }

void ParticleData::generateParticles(U32 particleCount, U32 optionsMask) {
    _lodLevel = 0;
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
    _alive.clear();

    if (_totalCount > 0) {
        if (bitCompare(_optionsMask, Properties::PROPERTIES_POS)) {
            _position.resize(_totalCount);
        }
        if (bitCompare(_optionsMask, Properties::PROPERTIES_VEL)) {
            _velocity.resize(_totalCount);
        }
        if (bitCompare(_optionsMask, Properties::PROPERTIES_ACC)) {
            _acceleration.resize(_totalCount);
        }
        if (bitCompare(_optionsMask, Properties::PROPERTIES_COLOR)) {
            _color.resize(_totalCount);
        }
        if (bitCompare(_optionsMask, Properties::PROPERTIES_COLOR_TRANS)) {
            _startColor.resize(_totalCount);
            _endColor.resize(_totalCount);
        }
        _misc.resize(_totalCount);
        _alive.resize(_totalCount);
    }
}

void ParticleData::kill(U32 index) {
    DIVIDE_ASSERT(index < _totalCount,
                  "ParticleData::kill error : Invalid index!");
    DIVIDE_ASSERT(_aliveCount > 0,
                  "ParticleData::kill error : No alive particles found!");

    _alive[index] = false;
    swapData(index, _aliveCount - 1);
    _aliveCount--;
}

void ParticleData::wake(U32 index) {
    DIVIDE_ASSERT(index < _totalCount,
                  "ParticleData::wake error : Invalid index!");
    DIVIDE_ASSERT(_aliveCount < _totalCount,
                  "ParticleData::wake error : No dead particles found!");

    _alive[index] = true;
    // swapData(index, _aliveCount);
    _aliveCount++;
}

void ParticleData::sort() {
    U32 count = aliveCount();

    _indices.resize(count);

    for (U32 i = 0; i < count; ++i) {
        _indices[i] = i;
    }

    std::sort(std::begin(_indices), std::end(_indices), IndexComp(*this));
}

void ParticleData::swapData(U32 indexA, U32 indexB) {
    if (bitCompare(_optionsMask, Properties::PROPERTIES_POS)) {
        // std::swap(_position[indexA], _position[indexB]);
        _position[indexA] = _position[indexB];
    }
    if (bitCompare(_optionsMask, Properties::PROPERTIES_VEL)) {
        // std::swap(_velocity[indexA], _velocity[indexB]);
        _velocity[indexA] = _velocity[indexB];
    }
    if (bitCompare(_optionsMask, Properties::PROPERTIES_ACC)) {
        // std::swap(_acceleration[indexA], _acceleration[indexB]);
        _acceleration[indexA] = _acceleration[indexB];
    }
    if (bitCompare(_optionsMask, Properties::PROPERTIES_COLOR)) {
        // std::swap(_color[indexA], _color[indexB]);
        _color[indexA] = _color[indexB];
    }
    if (bitCompare(_optionsMask, Properties::PROPERTIES_COLOR_TRANS)) {
        // std::swap(_startColor[indexA], _startColor[indexB]);
        _startColor[indexA] = _startColor[indexB];
        // std::swap(_endColor[indexA], _endColor[indexB]);
        _endColor[indexA] = _endColor[indexB];
    }
    // std::swap(_misc[indexA], _misc[indexB]);
    _misc[indexA] = _misc[indexB];
    // std::swap(_alive[indexA], _alive[indexB]);
    //_alive[indexA] = _alive[indexB];
}
};