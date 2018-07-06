#include "stdafx.h"

#include "Headers/Pipeline.h"

namespace Divide {

size_t PipelineDescriptor::getHash() const {
    _hash = _stateHash;
    Util::Hash_combine(_hash, _multiSampleCount);
    Util::Hash_combine(_hash, _shaderProgramHandle);

    for (U8 i = 0; i < to_base(ShaderType::COUNT); ++i) {
        Util::Hash_combine(_hash, i);

        const vector<U32>& functions = _shaderFunctions[i];
        for (U32 function : functions) {
            Util::Hash_combine(_hash, function);
        }
    }

    return _hash;
}

bool PipelineDescriptor::operator==(const PipelineDescriptor &other) const {
    return _stateHash == other._stateHash &&
           _multiSampleCount == other._multiSampleCount &&
           _shaderFunctions == other._shaderFunctions &&
           _shaderProgramHandle == other._shaderProgramHandle;
}

bool PipelineDescriptor::operator!=(const PipelineDescriptor &other) const {
    return _stateHash != other._stateHash ||
           _multiSampleCount != other._multiSampleCount ||
           _shaderFunctions != other._shaderFunctions ||
           _shaderProgramHandle != other._shaderProgramHandle;
}

Pipeline::Pipeline(const PipelineDescriptor& descriptor)
    : _descriptor(descriptor)
{
}

bool Pipeline::operator==(const Pipeline &other) const {
    return _descriptor == other._descriptor;
}

bool Pipeline::operator!=(const Pipeline &other) const {
    return _descriptor != other._descriptor;
}

}; //namespace Divide