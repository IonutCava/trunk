#include "Headers/glLockManager.h"
#include <assert.h>

namespace Divide {

GLuint64 kOneSecondInNanoSeconds = 1000000000;

glLockManager::glLockManager(bool cpuUpdates) : _CPUUpdates(cpuUpdates),
                                                _defaultSync(nullptr),
                                                _defaultSyncState(GL_UNSIGNALED)

{
}

glLockManager::~glLockManager()
{
    if (_defaultSync != nullptr) {
        glDeleteSync(_defaultSync);
        _defaultSync = nullptr;
        _defaultSyncState = GL_UNSIGNALED;
    }
}

void glLockManager::Wait() {
    if (_defaultSync != nullptr && _defaultSyncState == GL_SIGNALED) {
        _defaultSyncState = wait(&_defaultSync, _defaultSyncState);
        glDeleteSync(_defaultSync);
        _defaultSync = nullptr;
    }
}
 
void glLockManager::Lock() {
    assert(_defaultSync == nullptr);

    if (_defaultSyncState != GL_SIGNALED) {
        _defaultSync =
            glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 
                        UnusedMask::GL_UNUSED_BIT);

        _defaultSyncState = GL_SIGNALED;
    }
}

GLenum glLockManager::wait(GLsync* syncObj, GLenum response) {
    if (_CPUUpdates) {
        GLuint64 waitDuration = 0;
        while (true) {
            DIVIDE_ASSERT(response != GL_ALREADY_SIGNALED,
                          "Not sure what to do here. Probably raise an "
                          "exception or something.");

            response = glClientWaitSync(*syncObj,
                                        GL_SYNC_FLUSH_COMMANDS_BIT,
                                        waitDuration);
            if (response == GL_CONDITION_SATISFIED ||
                response == GL_ALREADY_SIGNALED ||
                response == GL_ZERO) {
                return response;
            }

            DIVIDE_ASSERT(response != GL_WAIT_FAILED,
                          "Not sure what to do here. Probably raise an "
                          "exception or something.");
            waitDuration = kOneSecondInNanoSeconds;
        }
    } else {
        glWaitSync(*syncObj, UnusedMask::GL_UNUSED_BIT, GL_TIMEOUT_IGNORED);
        response = GL_UNSIGNALED;
    }

    return response;
}

};//namespace Divide