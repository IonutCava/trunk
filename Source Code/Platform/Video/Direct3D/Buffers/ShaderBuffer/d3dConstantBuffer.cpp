#include "stdafx.h"

#include "Headers/d3dConstantBuffer.h"
#include "Platform/Video/Headers/GFXDevice.h"

namespace Divide {

IMPLEMENT_CUSTOM_ALLOCATOR(d3dConstantBuffer, 0, 0)

d3dConstantBuffer::d3dConstantBuffer(GFXDevice& context,
                                     const ShaderBufferParams& params)
    : ShaderBuffer(context, params)
{
}

d3dConstantBuffer::~d3dConstantBuffer()
{
}

void d3dConstantBuffer::getData(ptrdiff_t offsetElementCount,
                                ptrdiff_t rangeElementCount,
                                bufferPtr result) const {
}

void d3dConstantBuffer::updateData(ptrdiff_t offsetElementCount,
                                   ptrdiff_t rangeElementCount,
                                   const bufferPtr data) {
}

bool d3dConstantBuffer::bindRange(U32 bindIndex,
                                  U32 offsetElementCount,
                                  U32 rangeElementCount) {
    return false;
}

bool d3dConstantBuffer::bind(U32 bindIndex) {
    return false;
}

void d3dConstantBuffer::addAtomicCounter(U32 sizeFactor) {
}

U32 d3dConstantBuffer::getAtomicCounter(U32 counterIndex) {
    return 0;
}

void d3dConstantBuffer::bindAtomicCounter(U32 counterIndex, U32 bindIndex) {
}

void d3dConstantBuffer::resetAtomicCounter(U32 counterIndex) {
}

};//namespace Divide