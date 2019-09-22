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
            glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, UnusedMask::GL_UNUSED_BIT)
        });
}

glGlobalLockManager::glGlobalLockManager() noexcept
{

}

glGlobalLockManager::~glGlobalLockManager()
{

}

GLsync glGlobalLockManager::syncHere() const {
    return glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, UnusedMask::GL_UNUSED_BIT);
}

bool glGlobalLockManager::test(GLsync syncObject, vectorEASTL<BufferRange>& ranges, BufferRange testRange, bool noWait) {
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
    bool foundLockedRange = false;
    {
        SharedLock r_lock(_lock);
        for (const auto& it : _bufferLocks) {
            if (it.second.find(bufferHandle) != std::cend(it.second)) {
                foundLockedRange = true;
                break;
            }
        }
    }

    if (foundLockedRange) {
        bool ret = false;

        BufferRange testRange{ lockBeginBytes, lockLength };

        UniqueLockShared w_lock(_lock);
        // Check again as the range may have been cleared on another thread
        for (auto it = eastl::begin(_bufferLocks); it != eastl::end(_bufferLocks);) {
            auto entry = it->second.find(bufferHandle);
            if (entry != std::cend(it->second)) {
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

void glGlobalLockManager::LockBuffers(BufferLockEntries&& entries, bool flush) {
    for (auto& it1 : entries) {
        for (auto& it2 : it1.second) {
            WaitForLockedRange(it1.first, it2._startOffset, it2._length, true);
        }
    }

    {
        UniqueLockShared w_lock(_lock);
        hashAlg::emplace(_bufferLocks, syncHere(), entries);
    }

    if (flush) {
        glFlush();
    }
}

};
