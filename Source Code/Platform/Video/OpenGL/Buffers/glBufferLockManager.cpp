#include "Headers/glBufferLockManager.h"

namespace Divide {

// --------------------------------------------------------------------------------------------------------------------
// --------------------------------------------------------------------------------------------------------------------
// --------------------------------------------------------------------------------------------------------------------
glBufferLockManager::glBufferLockManager()
    : glLockManager()
{
    _bufferLocks.reserve(128);
}

// --------------------------------------------------------------------------------------------------------------------
glBufferLockManager::~glBufferLockManager() {
    WriteLock w_lock(_lock);
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

    vectorImpl<BufferLock> swapLocks;

    WriteLock w_lock(_lock);
    for (BufferLock& lock : _bufferLocks) {
        if (testRange.Overlaps(lock._range)) {
            wait(&lock._syncObj, blockClient);
            cleanup(&lock);
        } else {
            swapLocks.push_back(lock);
        }
    }

    _bufferLocks.swap(swapLocks);
}

// --------------------------------------------------------------------------------------------------------------------
void glBufferLockManager::LockRange(size_t lockBeginBytes,
                                    size_t lockLength) {
    BufferRange testRange = { lockBeginBytes, lockLength };
    {
        WriteLock w_lock(_lock);
        for (BufferLock& lock : _bufferLocks) {
            if (testRange.Overlaps(lock._range)) {
                // no point in locking the same range multiple times
                return;
            }
        }

        _bufferLocks.push_back({testRange,
                                glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, UnusedMask::GL_UNUSED_BIT)
                               });
    }
}

// --------------------------------------------------------------------------------------------------------------------
void glBufferLockManager::cleanup(BufferLock* bufferLock) {
    glDeleteSync(bufferLock->_syncObj);
}

};