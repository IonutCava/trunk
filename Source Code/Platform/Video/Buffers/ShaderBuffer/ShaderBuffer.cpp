#include "stdafx.h"

#include "Headers/ShaderBuffer.h"
#include "Platform/Video/Headers/GFXDevice.h"

namespace Divide {

size_t ShaderBuffer::s_boundAlignmentRequirement = 0;
size_t ShaderBuffer::s_unboundAlignmentRequirement = 0;

ShaderBuffer::ShaderBuffer(GFXDevice& context,
                           const ShaderBufferDescriptor& descriptor)
      : GraphicsResource(context, Type::SHADER_BUFFER, getGUID(), _ID(descriptor._name.c_str())),
        RingBufferSeparateWrite(descriptor._ringBufferLength, descriptor._separateReadWrite),
        _elementSize(descriptor._elementSize),
        _elementCount(descriptor._elementCount),
        _flags(descriptor._flags),
        _usage(descriptor._usage),
        _frequency(descriptor._updateFrequency),
        _updateUsage(descriptor._updateUsage),
        _name(descriptor._name)
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

size_t ShaderBuffer::alignmentRequirement(const Usage usage) {
    return usage == Usage::CONSTANT_BUFFER ? s_boundAlignmentRequirement : 
                    usage == Usage::UNBOUND_BUFFER ? s_unboundAlignmentRequirement : sizeof(U32);
}

}; //namespace Divide;