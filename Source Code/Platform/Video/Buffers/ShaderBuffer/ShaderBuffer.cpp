#include "stdafx.h"

#include "config.h"

#include "Headers/ShaderBuffer.h"
#include "Platform/Video/Headers/GFXDevice.h"

namespace Divide {

size_t ShaderBuffer::_boundAlignmentRequirement = 0;
size_t ShaderBuffer::_unboundAlignmentRequirement = 0;

ShaderBuffer::ShaderBuffer(GFXDevice& context,
                           const ShaderBufferDescriptor& descriptor)
      : GraphicsResource(context, GraphicsResource::Type::SHADER_BUFFER, getGUID()),
        RingBufferSeparateWrite(descriptor._ringBufferLength, descriptor._separateReadWrite),
        _elementCount(descriptor._elementCount),
        _frequency(descriptor._updateFrequency),
        _flags(descriptor._flags),
        _name(descriptor._name),
        _bufferSize(0),
        _maxSize(0)
{
    _bufferSize = descriptor._elementSize * _elementCount;
    assert(_bufferSize > 0 && "ShaderBuffer::Create error: Invalid buffer size!");
}

ShaderBuffer::~ShaderBuffer()
{
}

void ShaderBuffer::writeData(const bufferPtr data) {
    writeData(0, _elementCount, data);
}

size_t ShaderBuffer::alignmentRequirement(bool unbound) {
    return unbound ? _unboundAlignmentRequirement : _boundAlignmentRequirement;
}

}; //namespace Divide;