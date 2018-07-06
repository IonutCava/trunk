#include "config.h"

#include "Headers/ShaderBuffer.h"
#include "Platform/Video/Headers/GFXDevice.h"

namespace Divide {

size_t ShaderBuffer::_boundAlignmentRequirement = 0;
size_t ShaderBuffer::_unboundAlignmentRequirement = 0;

ShaderBuffer::ShaderBuffer(GFXDevice& context,
                            const ShaderBufferParams& params)
    : GUIDWrapper(),
        GraphicsResource(context, getGUID()),
        RingBuffer(params._ringBufferLength),
        _primitiveSize(params._primitiveSizeInBytes),
        _primitiveCount(params._primitiveCount),
        _frequency(params._updateFrequency),
        _unbound(params._unbound),
        _bufferSize(0),
        _maxSize(0)
{
    _bufferSize = _primitiveSize * _primitiveCount;
    assert(_bufferSize > 0 && "ShaderBuffer::Create error: Invalid buffer size!");
}

ShaderBuffer::~ShaderBuffer()
{
}

void ShaderBuffer::setData(const bufferPtr data) {
    assert(_bufferSize > 0 && "ShaderBuffer::SetData error: Invalid buffer size!");
    updateData(0, _primitiveCount, data);
}

size_t ShaderBuffer::alignmentRequirement(bool unbound) {
    return unbound ? _unboundAlignmentRequirement : _boundAlignmentRequirement;
}
}; //namespace Divide;