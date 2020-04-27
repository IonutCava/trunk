#include "stdafx.h"

#include "Headers/glBufferLockManager.h"

#include "Platform/Headers/PlatformRuntime.h"
#include "Platform/Video/RenderBackend/OpenGL/Headers/GLWrapper.h"

#include <eastl/fixed_set.h>

namespace Divide {

namespace {
    // Auto-delete locks older than this number of frames
    constexpr U32 g_LockFrameLifetime = 6u; //(APP->Driver->GPU x2)
};

// --------------------------------------------------------------------------------------------------------------------
// --------------------------------------------------------------------------------------------------------------------
// --------------------------------------------------------------------------------------------------------------------
glBufferLockManager::glBufferLockManager()
    : glLockManager()
{
    _swapLocks.reserve(32);
    _bufferLocks.reserve(32);
}

// --------------------------------------------------------------------------------------------------------------------
glBufferLockManager::~glBufferLockManager() {
    const UniqueLock<Mutex> w_lock(_lock);
    for (const BufferLock& lock : _bufferLocks) {
        glDeleteSync(lock._syncObj);
    }

    _bufferLocks.clear();
}

// --------------------------------------------------------------------------------------------------------------------
bool glBufferLockManager::WaitForLockedRange(size_t lockBeginBytes,
                                             size_t lockLength,
                                             bool blockClient,
                                             bool quickCheck) {
    OPTICK_EVENT();
    OPTICK_TAG("BlockClient", blockClient);
    OPTICK_TAG("QuickCheck", quickCheck);

    const BufferRange testRange{lockBeginBytes, lockLength};

    bool error = false;
    UniqueLock<Mutex> w_lock(_lock);
    _swapLocks.resize(0);
    for (BufferLock& lock : _bufferLocks) {
        if (!lock._valid || testRange.Overlaps(lock._range)) {
            U8 retryCount = 0;
            if (!lock._valid || wait(lock._syncObj, blockClient, quickCheck, retryCount)) {
                glDeleteSync(lock._syncObj);
                if (retryCount > 4) {
                    Console::d_errorfn("glBufferLockManager: Wait (%p) [%d - %d] %s - %d retries", this, lockBeginBytes, lockLength, blockClient ? "true" : "false", retryCount);
                }
            } else if (!quickCheck) {
                error = true;
            }
        } else {
            _swapLocks.push_back(lock);
        }
    }

    _bufferLocks.swap(_swapLocks);

    return !error;
}

// --------------------------------------------------------------------------------------------------------------------
void glBufferLockManager::LockRange(size_t lockBeginBytes, size_t lockLength, U32 frameID) {
    OPTICK_EVENT();

    {//Delete old lock entries
        UniqueLock<Mutex> w_lock(_lock);
        for (BufferLock& lock : _bufferLocks) {
            if (frameID - lock._frameID >= g_LockFrameLifetime) {
                lock._valid = false;
            }
        }
    }

    WaitForLockedRange(lockBeginBytes, lockLength, true, true);


    BufferLock newLock = {};
    newLock._range = { lockBeginBytes, lockLength };
    newLock._frameID = frameID;
    newLock._valid = true;
    newLock._syncObj = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);

    {
        UniqueLock<Mutex> w_lock(_lock);
        _bufferLocks.push_back(newLock);
    }
}

};
