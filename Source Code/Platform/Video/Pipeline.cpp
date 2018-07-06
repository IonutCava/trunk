#include "stdafx.h"

#include "Headers/Pipeline.h"

namespace Divide
{

namespace
{
    constexpr size_t DEFAULT_DESCRIPTOR_HASH = 123456789;
};

size_t PipelineDescriptor::computeHash() const {
    size_t hash = DEFAULT_DESCRIPTOR_HASH;
    Util::Hash_combine(hash, _stateHash);
    Util::Hash_combine(hash, _multiSampleCount);
    if (_shaderProgram != nullptr) {
        Util::Hash_combine(hash, _shaderProgram->getID());
    }

    return hash;
}

Pipeline::Pipeline()
    : _descriptorHash(DEFAULT_DESCRIPTOR_HASH)
    , _stateHash(0)
    , _shaderProgram(nullptr)
    , _multiSampleCount(0)
{

}

Pipeline::Pipeline(const PipelineDescriptor& descriptor)
    : _descriptorHash(descriptor.computeHash())
    , _stateHash(descriptor._stateHash)
    , _shaderProgram(descriptor._shaderProgram)
    , _multiSampleCount(descriptor._multiSampleCount)
{
}

Pipeline::~Pipeline()
{
}

void Pipeline::fromDescriptor(const PipelineDescriptor& descriptor) {
    _descriptorHash = descriptor.computeHash();
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
    return _descriptorHash == other._descriptorHash;
}

bool Pipeline::operator!=(const Pipeline &other) const {
    return _descriptorHash != other._descriptorHash;
}

}; //namespace Divide