/*
Copyright (c) 2018 DIVIDE-Studio
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

#pragma once
#ifndef _CORE_RING_BUFFER_H_
#define _CORE_RING_BUFFER_H_

namespace Divide {

class RingBufferSeparateWrite {
public:
    // If separateReadWrite is true, this behaves exactly like a RingBuffer
    explicit RingBufferSeparateWrite(I32 queueLength, bool separateReadWrite);
    RingBufferSeparateWrite(const RingBufferSeparateWrite& other);
    virtual ~RingBufferSeparateWrite();

    RingBufferSeparateWrite& operator=(const RingBufferSeparateWrite& other);

    virtual void resize(I32 queueLength);

    const inline I32 queueLength() const {
        return _queueLength;
    }

    const inline I32 queueWriteIndex() const {
        return _separateReadWrite ? (_queueIndex + (_queueLength - 1)) % _queueLength 
                                  : _queueIndex;
    }

    const inline I32 queueReadIndex() const {
        return _queueIndex;
    }

    inline void incQueue() {
        if (queueLength() > 1) {
            _queueIndex = (_queueIndex + 1) % _queueLength;
        }
    }

    inline void decQueue() {
        if (queueLength() > 1) {
            if (_queueIndex == 0) {
                _queueIndex = _queueLength;
            }

            _queueIndex = (_queueIndex - 1) % _queueLength;
        }
    }

private:
    I32 _queueLength;
    bool _separateReadWrite;
    std::atomic_int _queueIndex;
};

class RingBuffer {
public:
    explicit RingBuffer(I32 queueLength);
    RingBuffer(const RingBuffer& other);
    virtual ~RingBuffer();

    RingBuffer& operator=(const RingBuffer& other);

    virtual void resize(I32 queueLength);

    const inline I32 queueLength() const {
        return _queueLength;
    }

    const inline I32 queueIndex() const {
        return _queueIndex;
    }

    inline void incQueue() {
        if (queueLength() > 1) {
            _queueIndex = (_queueIndex + 1) % _queueLength;
        }
    }

    inline void decQueue() {
        if (queueLength() > 1) {
            _queueIndex = (_queueIndex - 1) % _queueLength;
        }
    }

private:
    I32 _queueLength;
    std::atomic_int _queueIndex;
};

}; //namespace Divide

#endif //_CORE_RING_BUFFER_H_