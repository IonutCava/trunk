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
    GLintptr _startOffset = 0;
    GLsizeiptr _length = 0;

    inline bool Overlaps(const BufferRange& _rhs) const {
        return static_cast<GLsizeiptr>(_startOffset) < (_rhs._startOffset + _rhs._length) &&
               static_cast<GLsizeiptr>(_rhs._startOffset) < (_startOffset + _length);
    }
};

// --------------------------------------------------------------------------------------------------------------------
struct BufferLock {
    BufferRange _range = {};
    GLsync _syncObj = nullptr;
};

// --------------------------------------------------------------------------------------------------------------------
class glBufferLockManager : public glLockManager {
   public:
    glBufferLockManager() noexcept;
    ~glBufferLockManager();

    // Return true if we found a lock to wait on
    bool WaitForLockedRange(GLintptr lockBeginBytes, GLsizeiptr lockLength, bool blockClient, bool quickCheck = false);
    void LockRange(GLintptr lockBeginBytes, GLsizeiptr lockLength);

   private:
    mutable std::mutex _lock;
    vectorEASTL<BufferLock> _bufferLocks;
    vectorEASTL<BufferLock> _swapLocks;
};

using LockEntries = vectorEASTL<BufferRange>;
using BufferLockEntries = hashMap<GLuint /*buffer handle*/,  LockEntries>;

class glGlobalLockManager : public glLockManager {
public:
    glGlobalLockManager() noexcept;
    ~glGlobalLockManager();

    void clean(U32 frameID);

    // Return true if  we found a lock to wait on
    bool WaitForLockedRange(GLuint bufferHandle, GLintptr lockBeginBytes, GLsizeiptr lockLength, bool noWait = false);
    void LockBuffers(BufferLockEntries&& entries, bool flush, U32 frameID);

    inline size_t lastTotalLockCount() const noexcept { return _lockCount; }

protected:
    bool test(GLsync syncObject, const vectorEASTL<BufferRange>& ranges, const BufferRange& testRange, bool noWait = false);
    void quickCheckOldEntries(U32 frameID);

    void cleanLocked(U32 frameID);
private:
    size_t _lockCount = 0;

    mutable SharedMutex _lock;
    using LockMap = hashMap<GLsync, std::pair<BufferLockEntries, U32>>;
    LockMap _bufferLocks;
};

};  // namespace Divide

#endif
