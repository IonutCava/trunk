#include "stdafx.h"

#include "Headers/Pipeline.h"

namespace Divide
{

Pipeline::Pipeline()
    : _stateHash(0)
    , _multiSampleCount(0)
{

}

Pipeline::Pipeline(const PipelineDescriptor& descriptor)
    : _stateHash(descriptor._stateHash)
    , _shaderProgram(descriptor._shaderProgram)
    , _multiSampleCount(descriptor._multiSampleCount)
{
}

Pipeline::~Pipeline()
{
}

void Pipeline::fromDescriptor(const PipelineDescriptor& descriptor) {
    _stateHash = descriptor._stateHash;
    _shaderProgram = descriptor._shaderProgram;
    _multiSampleCount = descriptor._multiSampleCount;
}

PipelineDescriptor Pipeline::toDescriptor() const {
    PipelineDescriptor desc;
    desc._multiSampleCount = _multiSampleCount;
    desc._shaderProgram = _shaderProgram;
    desc._stateHash = _stateHash;

    return desc;
}

bool Pipeline::operator==(const Pipeline &other) const {
    return _stateHash == other._stateHash &&
           _multiSampleCount == other._multiSampleCount &&
           (_shaderProgram.expired() ? 0 : _shaderProgram.lock()->getID())
                    == (other._shaderProgram.expired() ? 0 : other._shaderProgram.lock()->getID());
}

bool Pipeline::operator!=(const Pipeline &other) const {
    return _stateHash != other._stateHash ||
           _multiSampleCount != other._multiSampleCount ||
            (_shaderProgram.expired() ? 0 : _shaderProgram.lock()->getID())
                    != (other._shaderProgram.expired() ? 0 : other._shaderProgram.lock()->getID());
}

size_t Pipeline::getHash() const {
    size_t hash = _stateHash;
    Util::Hash_combine(hash, _multiSampleCount);
    if (!_shaderProgram.expired()) {
        Util::Hash_combine(hash, _shaderProgram.lock()->getID());
    }

    return hash;
}
}; //namespace Divide