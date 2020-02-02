#include "stdafx.h"

#include "Headers/glLockManager.h"

#include "Platform/Headers/PlatformRuntime.h"
#include "Platform/Video/RenderBackend/OpenGL/Headers/GLWrapper.h"

namespace Divide {

GLuint64 kOneSecondInNanoSeconds = 1000000000;
U8 kMaxWaitRetry = 5;

glLockManager::glLockManager() noexcept
    : GUIDWrapper(),
     _defaultSync(nullptr)
{
}

glLockManager::~glLockManager()
{
    Wait(false);
}

void glLockManager::Wait(bool blockClient) {
    OPTICK_EVENT();

    {
        SharedLock r_lock(_syncMutex);
        if (_defaultSync == nullptr) {
            return;
        }
    }

    UniqueLockShared w_lock(_syncMutex);
    if (_defaultSync != nullptr) {
        wait(_defaultSync, blockClient);
        glDeleteSync(_defaultSync);
        _defaultSync = nullptr;
    }
}
 
void glLockManager::Lock(bool flush) {
    OPTICK_EVENT();

    UniqueLockShared lock(_syncMutex);
    assert(_defaultSync == nullptr);
    _defaultSync = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
    // A glFlush call is needed after creating a new fence to make sure we don't end up with an infinite wait issue
    if (flush) {
        glFlush();
    }
}

bool glLockManager::wait(GLsync syncObj, bool blockClient, bool quickCheck, U8& retryCount) {
    OPTICK_EVENT();
    OPTICK_TAG("Blocking", blockClient);
    OPTICK_TAG("QuickCheck", quickCheck);
    OPTICK_TAG("RetryCount", retryCount);

    if (blockClient) {
        SyncObjectMask waitFlags = SyncObjectMask::GL_NONE_BIT;
        while (true) {
            OPTICK_EVENT("Wait - OnLoop");
            OPTICK_TAG("RetryCount", retryCount);
            const GLenum waitRet = glClientWaitSync(syncObj, waitFlags, retryCount > 2 ? kOneSecondInNanoSeconds : 0);
            if (waitRet == GL_ALREADY_SIGNALED || waitRet == GL_CONDITION_SATISFIED) {
                return true;
            }

            if (quickCheck) {
                return false;
            }

            DIVIDE_ASSERT(waitRet != GL_WAIT_FAILED, "glLockManager::wait error: Not sure what to do here. Probably raise an exception or something.");

            // After the first time, need to start flushing, and wait for a looong time. Only flush once though
             waitFlags = (retryCount % 2 == 1) ? GL_SYNC_FLUSH_COMMANDS_BIT : SyncObjectMask::GL_NONE_BIT;

            if (++retryCount > kMaxWaitRetry) {
                if (waitRet != GL_TIMEOUT_EXPIRED) {
                    Console::errorfn("glLockManager::wait error: Lock timeout");
                }

                return false;
            }
        }
    } else {
        glWaitSync(syncObj, 0, GL_TIMEOUT_IGNORED);
    }

    return true;
}

};//namespace Divide