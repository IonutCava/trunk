#include "stdafx.h"

#include "Headers/d3dConstantBuffer.h"
#include "Platform/Video/Headers/GFXDevice.h"

namespace Divide {

IMPLEMENT_CUSTOM_ALLOCATOR(d3dConstantBuffer, 0, 0)

d3dConstantBuffer::d3dConstantBuffer(GFXDevice& context,
                                     const ShaderBufferDescriptor& descriptor)
    : ShaderBuffer(context, descriptor)
{
}

d3dConstantBuffer::~d3dConstantBuffer()
{
}

void d3dConstantBuffer::readData(ptrdiff_t offsetElementCount,
                                 ptrdiff_t rangeElementCount,
                                 bufferPtr result) const {
    ACKNOWLEDGE_UNUSED(offsetElementCount);
    ACKNOWLEDGE_UNUSED(rangeElementCount);
    ACKNOWLEDGE_UNUSED(result);
}

void d3dConstantBuffer::writeData(ptrdiff_t offsetElementCount,
                                  ptrdiff_t rangeElementCount,
                                  const bufferPtr data) {
    ACKNOWLEDGE_UNUSED(offsetElementCount);
    ACKNOWLEDGE_UNUSED(rangeElementCount);
    ACKNOWLEDGE_UNUSED(data);
}

void d3dConstantBuffer::writeBytes(ptrdiff_t offsetInBytes,
                                   ptrdiff_t rangeInBytes,
                                   const bufferPtr data) {
    ACKNOWLEDGE_UNUSED(offsetInBytes);
    ACKNOWLEDGE_UNUSED(rangeInBytes);
    ACKNOWLEDGE_UNUSED(data);

}
bool d3dConstantBuffer::bindRange(U8 bindIndex,
                                  U32 offsetElementCount,
                                  U32 rangeElementCount) {
    ACKNOWLEDGE_UNUSED(bindIndex);
    ACKNOWLEDGE_UNUSED(offsetElementCount);
    ACKNOWLEDGE_UNUSED(rangeElementCount);
    return false;
}

bool d3dConstantBuffer::bind(U8 bindIndex) {
    ACKNOWLEDGE_UNUSED(bindIndex);
    return false;
}

void d3dConstantBuffer::addAtomicCounter(U32 sizeFactor, U16 ringSizeFactor) {
    ACKNOWLEDGE_UNUSED(sizeFactor);
    ACKNOWLEDGE_UNUSED(ringSizeFactor);
}

U32 d3dConstantBuffer::getAtomicCounter(U8 offset, U8 counterIndex) {
    ACKNOWLEDGE_UNUSED(counterIndex);
    return 0;
}

void d3dConstantBuffer::bindAtomicCounter(U8 offset, U8 counterIndex, U8 bindIndex) {
    ACKNOWLEDGE_UNUSED(counterIndex);
    ACKNOWLEDGE_UNUSED(bindIndex);
}

void d3dConstantBuffer::resetAtomicCounter(U8 offset, U8 counterIndex) {
    ACKNOWLEDGE_UNUSED(counterIndex);
}

};//namespace Divide