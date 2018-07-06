#include "Headers/glLockManager.h"
#include <assert.h>

namespace Divide {

GLuint64 kOneSecondInNanoSeconds = 1000000000;

glLockManager::glLockManager(bool cpuUpdates) : _CPUUpdates(cpuUpdates),
                                                _defaultSync(nullptr)
{
}

glLockManager::~glLockManager()
{
    Wait();
}

void glLockManager::Wait() {
    if (_defaultSync != nullptr) {
        wait(&_defaultSync);
        glDeleteSync(_defaultSync);
        _defaultSync = nullptr;
    }
}
 
void glLockManager::Lock() {
    assert(_defaultSync == nullptr);
    _defaultSync = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, UnusedMask::GL_UNUSED_BIT);
}

void glLockManager::wait(GLsync* syncObj) {
    if (_CPUUpdates) {
        SyncObjectMask waitFlags = SyncObjectMask::GL_NONE_BIT;
        GLuint64 waitDuration = 0;
        while (true) {
            GLenum waitRet = glClientWaitSync(*syncObj, waitFlags, waitDuration);
            if (waitRet == GL_ALREADY_SIGNALED || waitRet == GL_CONDITION_SATISFIED) {
                return;
            }
            
            DIVIDE_ASSERT(waitRet != GL_WAIT_FAILED,
                          "Not sure what to do here. Probably raise an "
                          "exception or something.");

            // After the first time, need to start flushing, and wait for a looong time.
            waitFlags = GL_SYNC_FLUSH_COMMANDS_BIT;
            waitDuration = kOneSecondInNanoSeconds;
        }
    } else {
        glWaitSync(*syncObj, UnusedMask::GL_UNUSED_BIT, GL_TIMEOUT_IGNORED);
    }
}

};//namespace Divide