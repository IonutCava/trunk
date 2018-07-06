/*
Copyright (c) 2017 DIVIDE-Studio
Copyright (c) 2009 Ionut Cava

This file is part of DIVIDE Framework.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software
and associated documentation files (the "Software"), to deal in the Software
without restriction,
including without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom the
Software is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED,
INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY,
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR
IN CONNECTION WITH THE SOFTWARE
OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

*/

#ifndef _CORE_RING_BUFFER_H_
#define _CORE_RING_BUFFER_H_

#include "Platform/Headers/PlatformDataTypes.h"
#include <atomic>

namespace Divide {

class RingBuffer {
public:
    explicit RingBuffer(U32 queueLength);
    RingBuffer(const RingBuffer& other);
    virtual ~RingBuffer();

    const inline U32 queueLength() const {
        return _queueLength;
    }

    const inline U32 queueWriteIndex() const {
        return _queueWriteIndex;
    }

    const inline U32 queueReadIndex() const {
        return _queueReadIndex;
    }

    inline void incQueue() {
        if (queueLength() > 1) {
            _queueWriteIndex = (_queueWriteIndex + 1) % _queueLength;
            _queueReadIndex = (_queueReadIndex + 1) % _queueLength;
        }
    }

    inline void decQueue() {
        if (queueLength() > 1) {
            _queueWriteIndex = (_queueWriteIndex - 1) % _queueLength;
            _queueReadIndex = (_queueReadIndex - 1) % _queueLength;
        }
    }

private:
    const U32 _queueLength;
    std::atomic_uint _queueReadIndex;
    std::atomic_uint _queueWriteIndex;
};

}; //namespace Divide

#endif //_CORE_RING_BUFFER_H_