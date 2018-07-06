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
    if (_defaultSync) {
        glDeleteSync(_defaultSync);
    }
}

void glLockManager::Wait() {
    if (_defaultSync) {
        GLenum retValue;
        wait(&_defaultSync, retValue);
    }
}
 
void glLockManager::Lock() {
    assert(_defaultSync == nullptr);

    _defaultSync =
        glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 
                    UnusedMask::GL_UNUSED_BIT);
}

void glLockManager::wait(GLsync* syncObj, GLenum& response) {
    if (_CPUUpdates) {
        GLuint64 waitDuration = 0;
        while (true) {
            response = glClientWaitSync(
                *syncObj, GL_SYNC_FLUSH_COMMANDS_BIT, waitDuration);
            if (response == GL_ALREADY_SIGNALED ||
                response == GL_CONDITION_SATISFIED) {
                return;
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
}

};//namespace Divide