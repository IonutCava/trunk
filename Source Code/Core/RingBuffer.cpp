#include "stdafx.h"

#include "Headers/RingBuffer.h"

namespace Divide {

RingBufferSeparateWrite::RingBufferSeparateWrite(I32 queueLength, bool separateReadWrite, bool writeAhead) noexcept :
        _queueLength(std::max(queueLength, 1)),
        _separateReadWrite(separateReadWrite),
        _writeAhead(writeAhead)
{
    _queueIndex = 0;
}

void RingBufferSeparateWrite::resize(I32 queueLength) noexcept {
    if (_queueLength != std::max(queueLength, 1)) {
        _queueLength = std::max(queueLength, 1);
        _queueIndex = std::min(_queueIndex.load(), _queueLength - 1);
    }
}

RingBuffer::RingBuffer(I32 queueLength)  noexcept :
    _queueLength(std::max(queueLength, 1))
{
    _queueIndex = 0;
}

void RingBuffer::resize(I32 queueLength) noexcept {
    _queueLength = std::max(queueLength, 1);
    _queueIndex = 0;
}

}; //namespace Divide