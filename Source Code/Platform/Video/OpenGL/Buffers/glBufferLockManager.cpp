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
    for (vectorImpl<BufferLock>::iterator it = std::begin(_bufferLocks);
        it != std::end(_bufferLocks); ++it) {
        cleanup(&*it);
    }

    _bufferLocks.clear();
}

// --------------------------------------------------------------------------------------------------------------------
void glBufferLockManager::WaitForLockedRange(size_t _lockBeginBytes,
                                             size_t _lockLength) {
    BufferRange testRange = {_lockBeginBytes, _lockLength};
    vectorImpl<BufferLock> swapLocks;
    for (vectorImpl<BufferLock>::iterator it = std::begin(_bufferLocks);
         it != std::end(_bufferLocks); ++it) {
        if (testRange.Overlaps(it->_range)) {
            wait(&it->_syncObj);
            cleanup(&*it);
        } else {
            swapLocks.push_back(*it);
        }
    }

    _bufferLocks.swap(swapLocks);
}

// --------------------------------------------------------------------------------------------------------------------
void glBufferLockManager::LockRange(size_t _lockBeginBytes,
                                    size_t _lockLength) {
    _bufferLocks.push_back({{_lockBeginBytes, _lockLength},
                            glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE,
                                        UnusedMask::GL_UNUSED_BIT)});
    _lastLockOffset = _lockBeginBytes;
    _lastLockRange = _lockLength;
}

// --------------------------------------------------------------------------------------------------------------------
void glBufferLockManager::cleanup(BufferLock* _bufferLock) {
    glDeleteSync(_bufferLock->_syncObj);
}
};