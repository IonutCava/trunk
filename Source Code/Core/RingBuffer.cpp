#include "stdafx.h"

#include "Headers/RingBuffer.h"

namespace Divide {

RingBuffer::RingBuffer(U32 queueLength) :
    _queueLength(std::max(queueLength, 1U))
{
    _queueReadIndex = 0;
    _queueWriteIndex = _queueLength - 1;
}

RingBuffer::RingBuffer(const RingBuffer& other)
    : _queueLength(other._queueLength)
{
    _queueReadIndex = other.queueReadIndex();
    _queueWriteIndex = other.queueWriteIndex();
}

RingBuffer::~RingBuffer()
{
}

RingBuffer& RingBuffer::operator=(const RingBuffer& other) {
    _queueLength = other._queueLength;
    _queueReadIndex = other.queueReadIndex();
    _queueWriteIndex = other.queueWriteIndex();

    return *this;
}

void RingBuffer::resize(U32 queueLength) {
    _queueLength = std::max(queueLength, 1U);
    _queueReadIndex = 0;
    _queueWriteIndex = _queueLength - 1;
}

}; //namespace Divide