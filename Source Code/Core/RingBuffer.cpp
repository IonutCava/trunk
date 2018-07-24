#include "stdafx.h"

#include "Headers/RingBuffer.h"

namespace Divide {

RingBufferSeparateWrite::RingBufferSeparateWrite(U32 queueLength) :
    _queueLength(std::max(queueLength, 1U))
{
    _queueReadIndex = 0;
    _queueWriteIndex = _queueLength - 1;
}

RingBufferSeparateWrite::RingBufferSeparateWrite(const RingBufferSeparateWrite& other)
    : _queueLength(other._queueLength)
{
    _queueReadIndex = other.queueReadIndex();
    _queueWriteIndex = other.queueWriteIndex();
}

RingBufferSeparateWrite::~RingBufferSeparateWrite()
{
}

RingBufferSeparateWrite& RingBufferSeparateWrite::operator=(const RingBufferSeparateWrite& other) {
    _queueLength = other._queueLength;
    _queueReadIndex = other.queueReadIndex();
    _queueWriteIndex = other.queueWriteIndex();

    return *this;
}

void RingBufferSeparateWrite::resize(U32 queueLength) {
    _queueLength = std::max(queueLength, 1U);
    _queueReadIndex = 0;
    _queueWriteIndex = _queueLength - 1;
}
RingBuffer::RingBuffer(U32 queueLength) :
    _queueLength(std::max(queueLength, 1U))
{
    _queueIndex = 0;
}

RingBuffer::RingBuffer(const RingBuffer& other)
    : _queueLength(other._queueLength)
{
    _queueIndex.store(other._queueIndex.load());
}

RingBuffer::~RingBuffer()
{
}

RingBuffer& RingBuffer::operator=(const RingBuffer& other) {
    _queueLength = other._queueLength;
    _queueIndex.store(other._queueIndex.load());

    return *this;
}

void RingBuffer::resize(U32 queueLength) {
    _queueLength = std::max(queueLength, 1U);
    _queueIndex = 0;
}

}; //namespace Divide