#include "stdafx.h"

#include "Headers/glBufferLockManager.h"
#include "Platform/Headers/PlatformRuntime.h"

namespace Divide {

// --------------------------------------------------------------------------------------------------------------------
// --------------------------------------------------------------------------------------------------------------------
// --------------------------------------------------------------------------------------------------------------------
glBufferLockManager::glBufferLockManager() noexcept
    : glLockManager()
{
    _swapLocks.reserve(32);
    _bufferLocks.reserve(32);
}

// --------------------------------------------------------------------------------------------------------------------
glBufferLockManager::~glBufferLockManager() {
    UniqueLock w_lock(_lock);
    for (BufferLock& lock : _bufferLocks) {
        cleanup(&lock);
    }

    _bufferLocks.clear();
}

// --------------------------------------------------------------------------------------------------------------------
void glBufferLockManager::WaitForLockedRange(size_t lockBeginBytes,
                                             size_t lockLength,
                                             bool blockClient) {
    BufferRange testRange = {lockBeginBytes, lockLength};

    _swapLocks.resize(0);

    UniqueLock w_lock(_lock);
    for (BufferLock& lock : _bufferLocks) {
        if (testRange.Overlaps(lock._range)) {
            wait(&lock._syncObj, blockClient);
            cleanup(&lock);
        } else {
            _swapLocks.push_back(lock);
        }
    }

    _bufferLocks.swap(_swapLocks);
}

// --------------------------------------------------------------------------------------------------------------------
void glBufferLockManager::LockRange(size_t lockBeginBytes,
                                    size_t lockLength) {
    GLsync sync = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, UnusedMask::GL_UNUSED_BIT);
    {
        UniqueLock w_lock(_lock);
        _bufferLocks.push_back(
            {
                {
                    lockBeginBytes,
                    lockLength
                },
                sync
            });
    }
    if (!Runtime::isMainThread()) {
        // Make forward progress in worker thread so that we don't deadlock
        glFlush();
    }
}

// --------------------------------------------------------------------------------------------------------------------
void glBufferLockManager::cleanup(BufferLock* bufferLock) {
    glDeleteSync(bufferLock->_syncObj);
}

};