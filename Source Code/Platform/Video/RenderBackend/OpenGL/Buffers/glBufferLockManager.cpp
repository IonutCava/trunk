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
    for (const BufferLock& lock : _bufferLocks) {
        glDeleteSync(lock._syncObj);
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
    const BufferRange testRange{lockBeginBytes, lockLength};

    UniqueLock w_lock(_lock);
    _swapLocks.resize(0);
    for (BufferLock& lock : _bufferLocks) {
        if (testRange.Overlaps(lock._range)) {
            U8 retryCount = 0;
            if (wait(lock._syncObj, blockClient, quickCheck, retryCount)) {
                glDeleteSync(lock._syncObj);
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
    UniqueLockShared w_lock(_lock);
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

            const BufferLockEntries::const_iterator& entry = lock._entries.find(bufferHandle);

            if (entry == std::cend(lock._entries)) {
                continue;
            }

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

    return ret;
}

bool glGlobalLockManager::quickCheckOldEntries(U32 frameID) {
    OPTICK_EVENT();

    bool ret = false;
    // Needed in order to delete old locks (from binds) that never needed actual waits (from writes). e.g. Terrain render nodes with static camera
    for (GLLockEntry& it  : _bufferLocks) {
        // Check how old these entries are.
        if (!it._valid || frameID - it._frameID > MAX_FRAME_AGE_BEFORE_AUTO_DELETE) {
            if (it._valid) {
                if (!wait(it._sync, true, true)) {
                    // ?
                }
                it._valid = false;
            }
            if (it._sync != nullptr) {
                glDeleteSync(it._sync);
            }
            ret = true;
        }
    }
    eastl::erase_if(_bufferLocks, [](const GLLockEntry& entry) { return !entry._valid; });
    return ret;
}

bool glGlobalLockManager::markOldDuplicateRangesAsInvalid(U32 frameID, const BufferLockEntries& entries) {
    OPTICK_EVENT();
    
    bool ret = false;
    for (GLLockEntry& bufferLock : _bufferLocks) {
        if (!bufferLock._valid) {
            continue;
        }

        for (auto& oldEntry : bufferLock._entries) {
            for (BufferRange& oldRange : oldEntry.second) {
                for (const auto& newEntry : entries) {
                    if (oldEntry.first != newEntry.first) {
                        continue;
                    }

                    for (const BufferRange& newRange : newEntry.second) {
                        if (oldRange.Overlaps(newRange)) {
                            bufferLock._valid = false;
                            ret = true;
                            goto NEXT_ENTRY;
                        }
                    }
                }
            }
        }
        NEXT_ENTRY:;
    }

    return ret;
}

void glGlobalLockManager::LockBuffers(BufferLockEntries&& entries, U32 frameID) {
    OPTICK_EVENT();

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
        if (markOldDuplicateRangesAsInvalid(frameID, sync._entries)) {
            if (quickCheckOldEntries(frameID)) {
                // ?
            }
        }
        assert(_bufferLocks.size() < MAX_LOCK_ENTRIES);
        _bufferLocks.push_back(sync);
    }
}

};
