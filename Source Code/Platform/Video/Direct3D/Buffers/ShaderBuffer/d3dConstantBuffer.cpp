#include "Headers/d3dConstantBuffer.h"

namespace Divide {

d3dConstantBuffer::d3dConstantBuffer(const stringImpl& bufferName,
                                     const U32 ringBufferLength,
                                     bool unbound,
                                     bool persistentMapped,
                                     BufferUpdateFrequency frequency)
    : ShaderBuffer(bufferName, ringBufferLength, unbound, persistentMapped, frequency)

{
}

d3dConstantBuffer::~d3dConstantBuffer()
{
}

void d3dConstantBuffer::destroy() {
}

void d3dConstantBuffer::create(U32 primitiveCount, ptrdiff_t primitiveSize, U32 sizeFactor) {
    ShaderBuffer::create(primitiveCount, primitiveSize, sizeFactor);
}

void d3dConstantBuffer::getData(ptrdiff_t offsetElementCount,
                                ptrdiff_t rangeElementCount,
                                bufferPtr result,
                                U32 sizeFactorOffset) const {
}

void d3dConstantBuffer::updateData(ptrdiff_t offsetElementCount,
                                   ptrdiff_t rangeElementCount,
                                   const bufferPtr data,
                                   U32 sizeFactorOffset) {
}

bool d3dConstantBuffer::bindRange(U32 bindIndex,
                                  U32 offsetElementCount,
                                  U32 rangeElementCount,
                                  U32 sizeFactorOffset) {
    return false;
}

bool d3dConstantBuffer::bind(U32 bindIndex, U32 sizeFactorOffset) {
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