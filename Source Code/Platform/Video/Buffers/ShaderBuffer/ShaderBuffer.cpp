#include "stdafx.h"

#include "config.h"

#include "Headers/ShaderBuffer.h"
#include "Platform/Video/Headers/GFXDevice.h"

namespace Divide {

size_t ShaderBuffer::s_boundAlignmentRequirement = 0;
size_t ShaderBuffer::s_unboundAlignmentRequirement = 0;

ShaderBuffer::ShaderBuffer(GFXDevice& context,
                           const ShaderBufferDescriptor& descriptor)
      : GraphicsResource(context, GraphicsResource::Type::SHADER_BUFFER, getGUID(), _ID(descriptor._name.c_str())),
        RingBufferSeparateWrite(descriptor._ringBufferLength, descriptor._separateReadWrite),
        _elementCount(descriptor._elementCount),
        _elementSize(descriptor._elementSize),
        _frequency(descriptor._updateFrequency),
        _flags(descriptor._flags),
        _usage(descriptor._usage),
        _name(descriptor._name),
        _bufferSize(0),
        _maxSize(0)
{
    _bufferSize = descriptor._elementSize * _elementCount;
    assert(descriptor._usage != Usage::COUNT);
    assert(_bufferSize > 0 && "ShaderBuffer::Create error: Invalid buffer size!");
}

ShaderBuffer::~ShaderBuffer()
{
}

void ShaderBuffer::writeData(const bufferPtr data) {
    writeData(0, _elementCount, data);
}

size_t ShaderBuffer::alignmentRequirement(Usage usage) {
    return usage == Usage::CONSTANT_BUFFER ? s_boundAlignmentRequirement : 
                    usage == Usage::UNBOUND_BUFFER ? s_unboundAlignmentRequirement : sizeof(U32);
}

}; //namespace Divide;