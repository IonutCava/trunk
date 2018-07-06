#include "stdafx.h"

#include "config.h"

#include "Headers/ShaderBuffer.h"
#include "Platform/Video/Headers/GFXDevice.h"

namespace Divide {

size_t ShaderBuffer::_boundAlignmentRequirement = 0;
size_t ShaderBuffer::_unboundAlignmentRequirement = 0;

ShaderBuffer::ShaderBuffer(GFXDevice& context,
                            const ShaderBufferDescriptor& descriptor)
    : GUIDWrapper(),
        GraphicsResource(context, getGUID()),
        RingBuffer(descriptor._ringBufferLength),
        _primitiveSize(descriptor._primitiveSizeInBytes),
        _primitiveCount(descriptor._primitiveCount),
        _frequency(descriptor._updateFrequency),
        _unbound(descriptor._unbound),
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