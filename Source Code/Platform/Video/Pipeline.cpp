#include "stdafx.h"

#include "Headers/Pipeline.h"

#include "Geometry/Material/Headers/ShaderComputeQueue.h"
#include "Scenes/Headers/SceneShaderData.h"

namespace Divide {

size_t PipelineDescriptor::getHash() const noexcept {
    _hash = _stateHash;
    Util::Hash_combine(_hash, _multiSampleCount);
    Util::Hash_combine(_hash, _shaderProgramHandle);

    for (U8 i = 0; i < to_base(ShaderType::COUNT); ++i) {
        Util::Hash_combine(_hash, i);
    }

    return _hash;
}

bool PipelineDescriptor::operator==(const PipelineDescriptor &other) const {
    return _stateHash == other._stateHash &&
           _multiSampleCount == other._multiSampleCount &&
           _shaderProgramHandle == other._shaderProgramHandle;
}

bool PipelineDescriptor::operator!=(const PipelineDescriptor &other) const {
    return _stateHash != other._stateHash ||
           _multiSampleCount != other._multiSampleCount ||
           _shaderProgramHandle != other._shaderProgramHandle;
}

Pipeline::Pipeline(const PipelineDescriptor& descriptor)
    : _cachedHash(descriptor.getHash()),
      _descriptor(descriptor)
{
}

}; //namespace Divide