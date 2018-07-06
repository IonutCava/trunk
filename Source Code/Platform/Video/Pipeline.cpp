#include "stdafx.h"

#include "Headers/Pipeline.h"

namespace Divide {

size_t PipelineDescriptor::getHash() const {
    _hash = _stateHash;
    Util::Hash_combine(_hash, _multiSampleCount);
    if (!_shaderProgram.expired()) {
        Util::Hash_combine(_hash, _shaderProgram.lock()->getID());
    }

    for (const ShaderFunctions::value_type& functions : _shaderFunctions) {
        Util::Hash_combine(_hash, functions.first);
        for (U32 function : functions.second) {
            Util::Hash_combine(_hash, function);
        }
    }

    return _hash;
}

Pipeline::Pipeline(const PipelineDescriptor& descriptor)
    : _descriptor(descriptor)
{
}

bool Pipeline::operator==(const Pipeline &other) const {
    return _descriptor._stateHash == other._descriptor._stateHash &&
           _descriptor._multiSampleCount == other._descriptor._multiSampleCount &&
           _descriptor._shaderFunctions == other._descriptor._shaderFunctions &&
           (_descriptor._shaderProgram.expired() ? 0 : _descriptor._shaderProgram.lock()->getID())
                    == (other._descriptor._shaderProgram.expired() ? 0 : other._descriptor._shaderProgram.lock()->getID());
}

bool Pipeline::operator!=(const Pipeline &other) const {
    return _descriptor._stateHash != other._descriptor._stateHash ||
           _descriptor._multiSampleCount != other._descriptor._multiSampleCount ||
           _descriptor._shaderFunctions != other._descriptor._shaderFunctions ||
            (_descriptor._shaderProgram.expired() ? 0 : _descriptor._shaderProgram.lock()->getID())
                    != (other._descriptor._shaderProgram.expired() ? 0 : other._descriptor._shaderProgram.lock()->getID());
}

}; //namespace Divide