#include "stdafx.h"

#include "Headers/glLockManager.h"
#include "Platform/Headers/PlatformRuntime.h"

namespace Divide {

GLuint64 kOneSecondInNanoSeconds = 1000000000;
U8 kMaxWaitRetry = 10;

glLockManager::glLockManager() noexcept
    : _defaultSync(nullptr)
{
}

glLockManager::~glLockManager()
{
    Wait(false);
}

void glLockManager::Wait(bool blockClient) {
    UniqueLock lock(_syncMutex);
    if (_defaultSync != nullptr) {
        wait(&_defaultSync, blockClient);
        glDeleteSync(_defaultSync);
        _defaultSync = nullptr;
    }
}
 
void glLockManager::Lock(bool flush) {
    assert(_defaultSync == nullptr);

    {
        UniqueLock lock(_syncMutex);
        _defaultSync = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, UnusedMask::GL_UNUSED_BIT);
    }
    // A glFlush call is needed after creating a new fence to make sure we don't end up with an infinite wait issue
    if (flush) {
        glFlush();
    }
}

void glLockManager::wait(GLsync* syncObj, bool blockClient) {
    if (blockClient) {
        SyncObjectMask waitFlags = SyncObjectMask::GL_NONE_BIT;
        U8 retryCount = 0;
        while (true) {
            GLenum waitRet = glClientWaitSync(*syncObj, waitFlags, retryCount > 1 ? kOneSecondInNanoSeconds : 0);
            if (waitRet == GL_ALREADY_SIGNALED || waitRet == GL_CONDITION_SATISFIED) {
                return;
            }

            DIVIDE_ASSERT(waitRet != GL_WAIT_FAILED, "glLockManager::wait error: Not sure what to do here. Probably raise an exception or something.");

            // After the first time, need to start flushing, and wait for a looong time.
            waitFlags = GL_SYNC_FLUSH_COMMANDS_BIT;

            if (++retryCount > kMaxWaitRetry) {
                DIVIDE_ASSERT(waitRet != GL_TIMEOUT_EXPIRED, "glLockManager::wait error: Lock timeout");
                return;
            }
        }
    } else {
        glWaitSync(*syncObj, UnusedMask::GL_UNUSED_BIT, GL_TIMEOUT_IGNORED);
    }
}

};//namespace Divide