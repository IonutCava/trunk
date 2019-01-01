#include "stdafx.h"

#include "Headers/RingBuffer.h"

namespace Divide {

RingBufferSeparateWrite::RingBufferSeparateWrite(I32 queueLength, bool separateReadWrite) :
        _queueLength(std::max(queueLength, 1)),
        _separateReadWrite(separateReadWrite)
{
    _queueIndex = 0;
}

RingBufferSeparateWrite::RingBufferSeparateWrite(const RingBufferSeparateWrite& other)
    : _queueLength(other._queueLength),
      _separateReadWrite(other._separateReadWrite)
{
    _queueIndex = other.queueReadIndex();
}

RingBufferSeparateWrite::~RingBufferSeparateWrite()
{
}

RingBufferSeparateWrite& RingBufferSeparateWrite::operator=(const RingBufferSeparateWrite& other) {
    _queueLength = other._queueLength;
    _separateReadWrite = other._separateReadWrite;
    _queueIndex = other.queueReadIndex();

    return *this;
}

void RingBufferSeparateWrite::resize(I32 queueLength) {
    if (_queueLength != std::max(queueLength, 1)) {
        _queueLength = std::max(queueLength, 1);
        _queueIndex = std::min(_queueIndex.load(), _queueLength - 1);
    }
}
RingBuffer::RingBuffer(I32 queueLength) :
    _queueLength(std::max(queueLength, 1))
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

void RingBuffer::resize(I32 queueLength) {
    _queueLength = std::max(queueLength, 1);
    _queueIndex = 0;
}

}; //namespace Divide