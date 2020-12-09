#include "stdafx.h"

#include "Headers/glLockManager.h"

#include "Platform/Video/RenderBackend/OpenGL/Headers/GLWrapper.h"

namespace Divide {

constexpr GLuint64 kOneSecondInNanoSeconds = 1000000000;
constexpr U8 kMaxWaitRetry = 5;

glLockManager::glLockManager() noexcept
    : GUIDWrapper(),
     _defaultSync(nullptr)
{
}

glLockManager::~glLockManager()
{
    Wait(false);
}

void glLockManager::Wait(const bool blockClient) {
    OPTICK_EVENT();

    {
        SharedLock<SharedMutex> r_lock(_syncMutex);
        if (_defaultSync == nullptr) {
            return;
        }
    }

    UniqueLock<SharedMutex> w_lock(_syncMutex);
    if (_defaultSync != nullptr) {
        wait(_defaultSync, blockClient);
        glDeleteSync(_defaultSync);
        _defaultSync = nullptr;
    }
}
 
void glLockManager::Lock(const bool flush) {
    OPTICK_EVENT();

    UniqueLock<SharedMutex> lock(_syncMutex);
    assert(_defaultSync == nullptr);
    _defaultSync = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
    // A glFlush call is needed after creating a new fence to make sure we don't end up with an infinite wait issue
    if (flush) {
        glFlush();
    }
}

bool glLockManager::wait(const GLsync syncObj, const bool blockClient, const bool quickCheck, U8& retryCount) {
    OPTICK_EVENT();
    OPTICK_TAG("Blocking", blockClient);
    OPTICK_TAG("QuickCheck", quickCheck);
    OPTICK_TAG("RetryCount", retryCount);

    if (!blockClient) {
        glWaitSync(syncObj, 0, GL_TIMEOUT_IGNORED);
        GL_API::QueueFlush();

        return true;
    }

    GLuint64 waitTimeout = 0u;
    SyncObjectMask waitFlags = SyncObjectMask::GL_NONE_BIT;
    while (true) {
        OPTICK_EVENT("Wait - OnLoop");
        OPTICK_TAG("RetryCount", retryCount);
        const GLenum waitRet = glClientWaitSync(syncObj, waitFlags, waitTimeout);
        if (waitRet == GL_ALREADY_SIGNALED || waitRet == GL_CONDITION_SATISFIED) {
            return true;
        }
        DIVIDE_ASSERT(waitRet != GL_WAIT_FAILED, "glLockManager::wait error: Not sure what to do here. Probably raise an exception or something.");

        if (quickCheck) {
            return false;
        }

        if (retryCount == 1) {
            //ToDo: Do I need this here? -Ionut
            GL_API::QueueFlush();
        }

        // After the first time, need to start flushing, and wait for a looong time.
        waitFlags = SyncObjectMask::GL_SYNC_FLUSH_COMMANDS_BIT;
        waitTimeout = kOneSecondInNanoSeconds;

        if (++retryCount > kMaxWaitRetry) {
            if (waitRet != GL_TIMEOUT_EXPIRED) {
                Console::errorfn("glLockManager::wait error: Lock timeout");
            }

            break;
        }
    }
    
    return false;
}

};//namespace Divide