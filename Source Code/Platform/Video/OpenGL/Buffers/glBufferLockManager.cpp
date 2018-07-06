#include "Headers/glBufferLockManager.h"

namespace Divide {

// --------------------------------------------------------------------------------------------------------------------
// --------------------------------------------------------------------------------------------------------------------
// --------------------------------------------------------------------------------------------------------------------
glBufferLockManager::glBufferLockManager(bool cpuUpdates)
    : glLockManager(cpuUpdates)
{
    _lastLockOffset = _lastLockRange = 0;
}

// --------------------------------------------------------------------------------------------------------------------
glBufferLockManager::~glBufferLockManager() {
    for (BufferLock& lock : _bufferLocks) {
        cleanup(&lock);
    }

    _bufferLocks.clear();
}

void glBufferLockManager::WaitForLockedRange() {
    if (_lastLockRange != 0) {
        WaitForLockedRange(_lastLockOffset, _lastLockRange);
    }
}

// --------------------------------------------------------------------------------------------------------------------
void glBufferLockManager::WaitForLockedRange(size_t _lockBeginBytes,
                                             size_t _lockLength) {
    BufferRange testRange = {_lockBeginBytes, _lockLength};

    vectorImpl<BufferLock> swapLocks;
    swapLocks.reserve(_bufferLocks.size());

    for (BufferLock& lock : _bufferLocks) {
        if (testRange.Overlaps(lock._range) &&
            lock._status == GL_SIGNALED) {
            lock._status = wait(&lock._syncObj, lock._status);
            cleanup(&lock);
        } else {
            swapLocks.push_back(lock);
        }
    }

    _bufferLocks.swap(swapLocks);
}

// --------------------------------------------------------------------------------------------------------------------
void glBufferLockManager::LockRange(size_t _lockBeginBytes,
                                    size_t _lockLength) {

    BufferRange testRange = { _lockBeginBytes, _lockLength };
    /*for (BufferLock& lock : _bufferLocks) {
        if (testRange.Overlaps(lock._range) && 
            lock._status == GL_SIGNALED) {
            WaitForLockedRange(_lockBeginBytes, _lockLength);
            break;
        }
    }*/

    _bufferLocks.push_back({testRange,
                            glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE,
                                        UnusedMask::GL_UNUSED_BIT), 
                            GL_SIGNALED});

    _lastLockOffset = _lockBeginBytes;
    _lastLockRange = _lockLength;
}

// --------------------------------------------------------------------------------------------------------------------
void glBufferLockManager::cleanup(BufferLock* _bufferLock) {
    glDeleteSync(_bufferLock->_syncObj);
    _bufferLock->_status = GL_UNSIGNALED;
    _bufferLock->_syncObj = nullptr;
}

};