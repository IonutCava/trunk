#include "stdafx.h"

#include "Headers/ShaderComputeQueue.h"

#include "Core/Headers/Kernel.h"
#include "Core/Time/Headers/ProfileTimer.h"

namespace Divide {

ShaderComputeQueue::ShaderComputeQueue(ResourceCache& cache)
    : _cache(cache),
      _queueComputeTimer(Time::ADD_TIMER("Shader Queue Timer"))
{
}

ShaderComputeQueue::~ShaderComputeQueue()
{
}

void ShaderComputeQueue::idle() {
    OPTICK_EVENT();

    constexpr U32 maxShadersPerCall = 32;
    {
        SharedLock r_lock(_queueLock);
        if (_shaderComputeQueue.empty()) {
            return;
        }
    }

    Time::ScopedTimer timer(_queueComputeTimer);

    U32 crtShaderCount = 0;
    UniqueLockShared lock(_queueLock);
    while (stepQueueLocked()) {
        ++_totalShaderComputeCount;
        if (++crtShaderCount == maxShadersPerCall) {
            break;
        }
    }
}

bool ShaderComputeQueue::stepQueue() {
    UniqueLockShared lock(_queueLock);
    return stepQueueLocked();
}

bool ShaderComputeQueue::stepQueueLocked() {
    if (_shaderComputeQueue.empty()) {
        return false;
    }

    ShaderQueueElement& currentItem = _shaderComputeQueue.front();
    currentItem._shaderDescriptor.waitForReady(false);
    currentItem._shaderRef = CreateResource<ShaderProgram>(_cache, currentItem._shaderDescriptor);
    _shaderComputeQueue.pop_front();
    return true;
}

void ShaderComputeQueue::addToQueueFront(const ShaderQueueElement& element) {
    UniqueLockShared w_lock(_queueLock);
    _shaderComputeQueue.push_front(element);
}

void ShaderComputeQueue::addToQueueBack(const ShaderQueueElement& element) {
    UniqueLockShared w_lock(_queueLock);
    _shaderComputeQueue.push_back(element);
}

}; //namespace Divide