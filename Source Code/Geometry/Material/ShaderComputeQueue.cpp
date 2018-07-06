#include "Headers/ShaderComputeQueue.h"

namespace Divide {

namespace {
#if defined(_DEBUG)
    const U32 g_MaxShadersComputedPerFrame = 4;
#else
    const U32 g_MaxShadersComputedPerFrame = 8;
#endif
};

ShaderComputeQueue::ShaderComputeQueue() : FrameListener()
{
    REGISTER_FRAME_LISTENER(this, 9999);
}

ShaderComputeQueue::~ShaderComputeQueue()
{
    UNREGISTER_FRAME_LISTENER(this);
}

void ShaderComputeQueue::update(const U64 deltaTime) {
    if (_shaderComputeQueue.empty() || _shadersComputedThisFrame) {
        return;
    }

    _totalShaderComputeCountThisFrame = 0;

    while(!_shaderComputeQueue.empty() &&
          _totalShaderComputeCountThisFrame++ < g_MaxShadersComputedPerFrame)
    {
        stepQueue();
    }

    _shadersComputedThisFrame = true;
}

void ShaderComputeQueue::stepQueue() {
    if (!_shaderComputeQueue.empty()) {
        const ShaderQueueElement& currentItem = _shaderComputeQueue.front();
        ShaderProgramInfo& info = *currentItem._shaderData;
        info._shaderRef = CreateResource<ShaderProgram>(currentItem._shaderDescriptor);
        info._shaderCompStage = ShaderProgramInfo::BuildStage::COMPUTED;
        _shaderComputeQueue.pop_front();
    }
}


void ShaderComputeQueue::addToQueueFront(const ShaderQueueElement& element) {
    _shaderComputeQueue.push_front(element);
}

void ShaderComputeQueue::addToQueueBack(const ShaderQueueElement& element) {
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