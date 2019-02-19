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
bool glBufferLockManager::WaitForLockedRange(size_t lockBeginBytes,
                                             size_t lockLength,
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
void glBufferLockManager::LockRange(size_t lockBeginBytes,
                                    size_t lockLength) {

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

bool glGlobalLockManager::WaitForLockedRange(I64 bufferGUID, size_t lockBeginBytes, size_t lockLength, bool noWait) {
    vectorEASTL<std::pair<GLsync, vectorEASTL<BufferRange>>> toTest = {};
    {
        SharedLock r_lock(_lock);
        for (auto it = eastl::begin(_bufferLocks); it != eastl::end(_bufferLocks); ++it) {
            const BufferLockEntries& entries = it->second;
            auto entry = entries.find(bufferGUID);
            if (entry != std::cend(entries)) {
                toTest.emplace_back(it->first, entry->second);
            }
        }
    }

    if (!toTest.empty()) {
        BufferRange testRange{ lockBeginBytes, lockLength };
        for (auto& it : toTest) {
            if (test(it.first, it.second, testRange, noWait)) {
                UniqueLockShared w_lock(_lock);
                _bufferLocks.erase(it.first);
            }
        }
        return true;
    }

    return false;
}

void glGlobalLockManager::LockBuffers(BufferLockEntries entries, bool flush) {
    for (auto& it1 : entries) {
        for (auto& it2 : it1.second) {
            WaitForLockedRange(it1.first, it2._startOffset, it2._length, true);
        }
    }

    GLsync syncObject = syncHere();
    {
        UniqueLockShared w_lock(_lock);
        hashAlg::emplace(_bufferLocks, syncObject, entries);
    }
    if (flush) {
        glFlush();
    }
}

};