#include "stdafx.h"

#include "Headers/ShaderComputeQueue.h"

#include "Core/Headers/Kernel.h"
#include "Core/Time/Headers/ProfileTimer.h"

namespace Divide {

namespace {
    const U32 g_MaxShadersComputedPerFrame = Config::Build::IS_DEBUG_BUILD ? 8 : 12;

    stringImpl getDefinesHash(const vector<std::pair<stringImpl, bool>>& defines) {
        size_t hash = 17;
        for (auto entry : defines) {
            Util::Hash_combine(hash, _ID(entry.first.c_str()));
            Util::Hash_combine(hash, entry.second);
        }
        return to_stringImpl(hash);
    }
    
};

ShaderComputeQueue::ShaderComputeQueue(ResourceCache& cache)
    : FrameListener(),
      _cache(cache),
      _queueComputeTimer(Time::ADD_TIMER("Shader Queue Timer"))
{
    REGISTER_FRAME_LISTENER(this, 9999);
}

ShaderComputeQueue::~ShaderComputeQueue()
{
    UNREGISTER_FRAME_LISTENER(this);
}

void ShaderComputeQueue::idle() {
    Time::ScopedTimer timer(_queueComputeTimer);

    if (_shadersComputedThisFrame) {
        return;
    }

    _totalShaderComputeCountThisFrame = 0;

    WAIT_FOR_CONDITION(!(stepQueue() &&
                         ++_totalShaderComputeCountThisFrame < g_MaxShadersComputedPerFrame));

    _shadersComputedThisFrame = _totalShaderComputeCountThisFrame > 0;
}

bool ShaderComputeQueue::stepQueue() {
    UniqueLock lock(_queueLock);
    if (!_shaderComputeQueue.empty()) {
        ShaderQueueElement& currentItem = _shaderComputeQueue.front();
        ShaderProgramInfo& info = *currentItem._shaderData;
        stringImpl resourceName = currentItem._shaderDescriptor.resourceName();
        currentItem._shaderDescriptor.assetName(resourceName);
        if (!info._shaderDefines.empty()) {
            currentItem._shaderDescriptor.resourceName(resourceName + "-" + getDefinesHash(info._shaderDefines));
        }
        
        info._shaderRef = CreateResource<ShaderProgram>(_cache, currentItem._shaderDescriptor);
        info.computeStage(ShaderProgramInfo::BuildStage::COMPUTED);
        _shaderComputeQueue.pop_front();
        return true;
    }

    return false;
}

void ShaderComputeQueue::addToQueueFront(const ShaderQueueElement& element) {
    UniqueLock w_lock(_queueLock);
    element._shaderData->computeStage(ShaderProgramInfo::BuildStage::QUEUED);
    _shaderComputeQueue.push_front(element);
}

void ShaderComputeQueue::addToQueueBack(const ShaderQueueElement& element) {
    UniqueLock w_lock(_queueLock);
    element._shaderData->computeStage(ShaderProgramInfo::BuildStage::QUEUED);
    _shaderComputeQueue.push_back(element);
}

bool ShaderComputeQueue::frameStarted(const FrameEvent& evt) {
    return true;
}

bool ShaderComputeQueue::frameEnded(const FrameEvent& evt) {
    if (_shadersComputedThisFrame) {
        _totalShaderComputeCount += _totalShaderComputeCountThisFrame;
        _totalShaderComputeCountThisFrame = 0;
        _shadersComputedThisFrame = false;
    }

    return true;
}

}; //namespace Divide