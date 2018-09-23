/*
Copyright (c) 2018 DIVIDE-Studio
Copyright (c) 2009 Ionut Cava

This file is part of DIVIDE Framework.

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software
and associated documentation files (the "Software"), to deal in the Software
without restriction,
including without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom the Software
is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED,
INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY,
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE
OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

*/

#pragma once
#ifndef _GL_BUFFER_LOCK_MANAGER_H
#define _GL_BUFFER_LOCK_MANAGER_H

#include "Platform/Video/RenderBackend/OpenGL/Headers/glLockManager.h"

// https://github.com/nvMcJohn/apitest
// --------------------------------------------------------------------------------------------------------------------
// --------------------------------------------------------------------------------------------------------------------
// --------------------------------------------------------------------------------------------------------------------

namespace Divide {

struct BufferRange {
    size_t _startOffset;
    size_t _length;

    inline bool Overlaps(const BufferRange& _rhs) const {
        return _startOffset < (_rhs._startOffset + _rhs._length) &&
               _rhs._startOffset < (_startOffset + _length);
    }
};

// --------------------------------------------------------------------------------------------------------------------
struct BufferLock {
    BufferRange _range;
    GLsync _syncObj;
};

// --------------------------------------------------------------------------------------------------------------------
class glBufferLockManager : public glLockManager {
   public:
    glBufferLockManager() noexcept;
    ~glBufferLockManager();

    void WaitForLockedRange(size_t lockBeginBytes, size_t lockLength, bool blockClient);
    void LockRange(size_t lockBeginBytes, size_t lockLength, bool flush);

   private:
    void cleanup(BufferLock* bufferLock);

   private:
    mutable std::mutex _lock;
    vector<BufferLock> _bufferLocks;
    vector<BufferLock> _swapLocks;
};

};  // namespace Divide

#endif
