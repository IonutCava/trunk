#include "Headers/glBufferLockManager.h"

namespace Divide {

GLuint64 kOneSecondInNanoSeconds = 1000000000;

// --------------------------------------------------------------------------------------------------------------------
// --------------------------------------------------------------------------------------------------------------------
// --------------------------------------------------------------------------------------------------------------------
glBufferLockManager::glBufferLockManager(bool _cpuUpdates)
: _CPUUpdates(_cpuUpdates)
{
    _lastLockOffset = _lastLockRange = 0;
}

// --------------------------------------------------------------------------------------------------------------------
glBufferLockManager::~glBufferLockManager()
{
    for (vectorImpl<BufferLock>::iterator it = std::begin(_bufferLocks); it != std::end(_bufferLocks); ++it) {
        cleanup(&*it);
    }

    _bufferLocks.clear();
}

// --------------------------------------------------------------------------------------------------------------------
void glBufferLockManager::WaitForLockedRange(size_t _lockBeginBytes, size_t _lockLength)
{
    BufferRange testRange = { _lockBeginBytes, _lockLength };
    vectorImpl<BufferLock> swapLocks;
    for (vectorImpl<BufferLock>::iterator it = std::begin(_bufferLocks); it != std::end(_bufferLocks); ++it)
    {
        if (testRange.Overlaps(it->mRange)) {
            wait(&it->mSyncObj);
            cleanup(&*it);
        } else {
            swapLocks.push_back(*it);
        }
    }

    _bufferLocks.swap(swapLocks);
}

// --------------------------------------------------------------------------------------------------------------------
void glBufferLockManager::LockRange(size_t _lockBeginBytes, size_t _lockLength)
{
    BufferRange newRange = { _lockBeginBytes, _lockLength };
    GLsync syncName = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
    BufferLock newLock = { newRange, syncName };

    _bufferLocks.push_back(newLock);

    _lastLockOffset = _lockBeginBytes;
    _lastLockRange = _lockLength;
}

// --------------------------------------------------------------------------------------------------------------------
void glBufferLockManager::wait(GLsync* _syncObj)
{
    if (_CPUUpdates) {
        GLbitfield waitFlags = 0;
        GLuint64 waitDuration = 0;
        while (1) {
            GLenum waitRet = glClientWaitSync(*_syncObj, waitFlags, waitDuration);
            if (waitRet == GL_ALREADY_SIGNALED || waitRet == GL_CONDITION_SATISFIED) {
                return;
            }

            DIVIDE_ASSERT(waitRet != GL_WAIT_FAILED, "Not sure what to do here. Probably raise an exception or something.");

            // After the first time, need to start flushing, and wait for a looong time.
            waitFlags = GL_SYNC_FLUSH_COMMANDS_BIT;
            waitDuration = kOneSecondInNanoSeconds;
        }
    } else {
        glWaitSync(*_syncObj, 0, GL_TIMEOUT_IGNORED);
    }
}

// --------------------------------------------------------------------------------------------------------------------
void glBufferLockManager::cleanup(BufferLock* _bufferLock)
{
    glDeleteSync(_bufferLock->mSyncObj);
}

};