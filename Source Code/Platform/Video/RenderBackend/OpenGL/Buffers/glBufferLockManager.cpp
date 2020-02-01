#include "stdafx.h"

#include "Headers/glBufferLockManager.h"

#include "Core/Headers/Console.h"
#include "Platform/Headers/PlatformRuntime.h"
#include "Platform/Video/RenderBackend/OpenGL/Headers/GLWrapper.h"

#include <eastl/fixed_set.h>

namespace Divide {

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
    const UniqueLock w_lock(_lock);
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
    OPTICK_EVENT();
    OPTICK_TAG("BlockClient", blockClient);
    OPTICK_TAG("QuickCheck", quickCheck);

    bool ret = false;
    BufferRange testRange{lockBeginBytes, lockLength};

    UniqueLock w_lock(_lock);
    _swapLocks.resize(0);
    for (BufferLock& lock : _bufferLocks) {
        if (testRange.Overlaps(lock._range)) {
            U8 retryCount = 0;
            if (wait(lock._syncObj, blockClient, quickCheck, retryCount)) {
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
    OPTICK_EVENT();

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

glGlobalLockManager::glGlobalLockManager()
{
    _lockIndex.store(0u);
}

void glGlobalLockManager::clean(U32 frameID) {
    quickCheckOldEntries(frameID);
}

struct GLLockEntryLight {
    GLsync* sync = nullptr;
    bool* valid = nullptr;
};

bool glGlobalLockManager::WaitForLockedRange(GLuint bufferHandle, GLintptr lockBeginBytes, GLsizeiptr lockLength, bool noWait) {
    OPTICK_EVENT();

    const U64 idx = _lockIndex.load();

    OPTICK_TAG("LockLength", to_U64(lockLength));
    OPTICK_TAG("noWait", noWait);
    OPTICK_TAG("LockIndex", idx);

    bool ret = false;

    const BufferRange testRange{ lockBeginBytes, lockLength };
    {
        SharedLock r_lock(_lock);
        // Check again as the range may have been cleared on another thread
        for (GLLockEntry& lock : _bufferLocks) {
            if (lock._ageID == idx || !lock._valid) {
                continue;
            }

            const BufferLockEntries& entries = lock._entries;
            const BufferLockEntries::const_iterator& entry = entries.find(bufferHandle);

            if (entry != std::cend(entries)) {
                for (const BufferRange& range : entry->second) {
                    if (testRange.Overlaps(range)) {
                        if (wait(lock._sync, true, noWait)) {
                            lock._valid = false;
                            ret = true;
                            break;
                        }
                    }
                }
            }
        }
    }

    return ret;
}

void glGlobalLockManager::quickCheckOldEntries(U32 frameID) {
    OPTICK_EVENT();

    UniqueLockShared w_lock(_lock);
    // Needed in order to delete old locks (from binds) that never needed actual waits (from writes). e.g. Terrain render nodes with static camera
    for (auto it = eastl::begin(_bufferLocks); it != eastl::end(_bufferLocks);) {
        // Check how old these entries are. Need to be at least 3 frames old for a fast check.
        if (!it->_valid || frameID - it->_frameID > 5) {
            if (it->_valid) {
                wait(it->_sync, true, true);
            }
            if (it->_sync != nullptr) {
                GL_API::registerSyncDelete(it->_sync);
            }
            it = _bufferLocks.erase(it);
        }else {
            ++it;
        }
    }
}

void glGlobalLockManager::markOldDuplicateRangesAsInvalid(U32 frameID, const BufferLockEntries& entries) {
    OPTICK_EVENT();
    
    for (auto& bufferLock : _bufferLocks) {
        if (bufferLock._valid) {
            for (auto& oldEntry : bufferLock._entries) {
                for (auto& newEntry : entries) {
                    if (oldEntry.first == newEntry.first) {
                        for (auto& oldRange : oldEntry.second) {
                            for (auto& newRange : newEntry.second) {
                                if (oldRange.Overlaps(newRange)) {
                                    bufferLock._valid = false;
                                    goto NEXT_ENTRY;
                                }
                            }
                        }
                    }
                }
            }
        }

        NEXT_ENTRY:;
    }
}

void glGlobalLockManager::LockBuffers(BufferLockEntries&& entries, bool flush, U32 frameID) {
    OPTICK_EVENT();
    OPTICK_TAG("Flush", flush);
    _lockIndex.fetch_add(1u);

    GLLockEntry sync = {};
    sync._ageID = _lockIndex.load();
    sync._entries = std::move(entries);
    sync._frameID = frameID;

    {
        OPTICK_EVENT("GL_SYNC!");
        sync._sync = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
    }
    {
        UniqueLockShared w_lock(_lock);
        markOldDuplicateRangesAsInvalid(frameID, sync._entries);
        assert(_bufferLocks.size() < MAX_LOCK_ENTRIES);
        _bufferLocks.push_back(sync);
    }

    if (flush) {
        OPTICK_EVENT("GL_FLUSH!");
        glFlush();
    }
}

};
