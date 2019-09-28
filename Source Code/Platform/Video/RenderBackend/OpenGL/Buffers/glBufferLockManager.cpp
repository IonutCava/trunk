#include "stdafx.h"

#include "Headers/glBufferLockManager.h"

#include "Core/Headers/Console.h"
#include "Platform/Headers/PlatformRuntime.h"
#include "Platform/Video/RenderBackend/OpenGL/Headers/GLWrapper.h"

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
        GL_API::registerSyncDelete(lock._syncObj);
    }

    _bufferLocks.clear();
}

// --------------------------------------------------------------------------------------------------------------------
bool glBufferLockManager::WaitForLockedRange(GLintptr lockBeginBytes,
                                             GLsizeiptr lockLength,
                                             bool blockClient,
                                             bool quickCheck) {

    bool ret = false;
    BufferRange testRange{lockBeginBytes, lockLength};

    UniqueLock w_lock(_lock);
    _swapLocks.resize(0);
    for (BufferLock& lock : _bufferLocks) {
        if (testRange.Overlaps(lock._range)) {
            U8 retryCount = 0;
            if (wait(&lock._syncObj, blockClient, quickCheck, retryCount)) {
                GL_API::registerSyncDelete(lock._syncObj);
                lock._syncObj = nullptr;
                if (retryCount > 0) {
                    //Console::d_errorfn("glBufferLockManager::WaitForLockedRange: Wait (%p) [%d - %d] %s - %d retries", this, lockBeginBytes, lockLength, blockClient ? "true" : "false", retryCount);
                }
                ret = true;
            }
        } else {
            _swapLocks.push_back(lock);
        }
    }

    _bufferLocks.swap(_swapLocks);

    return ret;
}

// --------------------------------------------------------------------------------------------------------------------
void glBufferLockManager::LockRange(GLintptr lockBeginBytes,
                                    GLsizeiptr lockLength) {

    if (WaitForLockedRange(lockBeginBytes, lockLength, true, true)) {
        //Console::printfn("Duplicate lock (%p) [%d - %d]", this, lockBeginBytes, lockLength);
    }

    UniqueLock w_lock(_lock);
    _bufferLocks.push_back(
        {
            {
                lockBeginBytes,
                lockLength
            },
            glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0)
        });
}

glGlobalLockManager::glGlobalLockManager() noexcept
    : _lockCount(0)
{
}

glGlobalLockManager::~glGlobalLockManager()
{
}

void glGlobalLockManager::clean(U32 frameID) {
    UniqueLockShared w_lock(_lock);
    cleanLocked(frameID);
}

void glGlobalLockManager::cleanLocked(U32 frameID) {
    // Delete all locks older than 6 frames  (APP->Driver->GPU x2)
    constexpr U32 LockDeleteFrameThreshold = 6;

    // Check again as the range may have been cleared on another thread
    for (auto it = eastl::begin(_bufferLocks); it != eastl::end(_bufferLocks);) {
        if (frameID - it->second.second > LockDeleteFrameThreshold) {
            GL_API::registerSyncDelete(it->first);
            it = _bufferLocks.erase(it);
        } else {
            ++it;
        }
    }
}

bool glGlobalLockManager::test(GLsync syncObject, const vectorEASTL<BufferRange>& ranges, const BufferRange& testRange, bool noWait) {
    for (const BufferRange& range : ranges) {
        if (testRange.Overlaps(range)) {
            U8 retryCount = 0;
            if (wait(&syncObject, true, noWait, retryCount)) {
                GL_API::registerSyncDelete(syncObject);
                syncObject = nullptr;
                if (retryCount > 1) {
                    //ToDo: do something?
                }
                return true;
            }
        }
    }

    return false;
}

bool glGlobalLockManager::WaitForLockedRange(GLuint bufferHandle, GLintptr lockBeginBytes, GLsizeiptr lockLength, bool noWait) {
    constexpr bool USE_PRESCAN = false;


    bool foundLockedRange = !USE_PRESCAN;
    if (USE_PRESCAN) {
        SharedLock r_lock(_lock);
        for (const auto& it : _bufferLocks) {
            if (it.second.first.find(bufferHandle) != std::cend(it.second.first)) {
                foundLockedRange = true;
                break;
            }
        }
    }

    if (foundLockedRange) {
        bool ret = false;

        const BufferRange testRange{ lockBeginBytes, lockLength };

        UniqueLockShared w_lock(_lock);
        _lockCount = 0;
        // Check again as the range may have been cleared on another thread
        for (auto it = eastl::begin(_bufferLocks); it != eastl::end(_bufferLocks);) {
            const BufferLockEntries& entries = it->second.first;
            _lockCount += entries.size();

            const auto& entry = entries.find(bufferHandle);
            if (entry != std::cend(entries)) {
                if (test(it->first, entry->second, testRange, noWait)) {
                    it = _bufferLocks.erase(it);
                } else {
                    ++it;
                }
                ret = true;
            } else {
                ++it;
            }
        }

        return ret;
    }
    
   return false;
}

void glGlobalLockManager::quickCheckOldEntries(U32 frameID) {
    // Needed in order to delete old locks (from binds) that never needed actual waits (from writes). e.g. Terrain render nodes with static camera
    for (auto it = eastl::begin(_bufferLocks); it != eastl::end(_bufferLocks);) {
        //Entries-FrameID
        const std::pair<BufferLockEntries, U32>& entriesForCrtBuffer = it->second;
        // Check how old these entries are. Need to be at least 3 frames old for a fast check.
        U32 frameAge = frameID - entriesForCrtBuffer.second;
        if (frameAge < 3) {
            ++it;
            continue;
        }
        GLsync syncObject = it->first;
        U8 retryCount = 0;
        if (frameAge > 5 || wait(&syncObject, true, true, retryCount)) {
            GL_API::registerSyncDelete(syncObject);
            it = _bufferLocks.erase(it);
        } else {
            ++it;
        }
    }
}

void glGlobalLockManager::LockBuffers(BufferLockEntries&& entries, bool flush, U32 frameID) {
    GLsync sync = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);

    {
        UniqueLockShared w_lock(_lock);
        quickCheckOldEntries(frameID);
        hashAlg::emplace(_bufferLocks, sync, std::make_pair(entries, frameID));
    }

    if (flush) {
        glFlush();
    }
}

};
