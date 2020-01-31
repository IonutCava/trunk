#include "stdafx.h"

#include "Headers/CommandBufferPool.h"

namespace Divide {
namespace GFX {


CommandBuffer* CommandBufferPool::allocateBuffer() {
    return _pool.newElement(_mutex);
}

void CommandBufferPool::deallocateBuffer(CommandBuffer*& buffer) {
    if (buffer != nullptr) {
        _pool.deleteElement(_mutex, buffer);
        buffer = nullptr;
    }
}

ScopedCommandBuffer::ScopedCommandBuffer(bool useSecondaryBuffers)
    : _useSecondaryBuffers(useSecondaryBuffers),
      _buffer(allocateCommandBuffer(useSecondaryBuffers))
{
}

ScopedCommandBuffer::~ScopedCommandBuffer()
{
    deallocateCommandBuffer(_buffer, _useSecondaryBuffers);
}


ScopedCommandBuffer allocateScopedCommandBuffer(bool useSecondaryBuffers) {
    OPTICK_EVENT();

    return ScopedCommandBuffer(useSecondaryBuffers);
}

CommandBuffer* allocateCommandBuffer(bool useSecondaryBuffers) {
    if (useSecondaryBuffers) {
        return s_secondaryCommandBufferPool.allocateBuffer();
    }
    return s_commandBufferPool.allocateBuffer();
}

void deallocateCommandBuffer(GFX::CommandBuffer*& buffer, bool useSecondaryBuffers) {
    if (useSecondaryBuffers) {
        return s_secondaryCommandBufferPool.deallocateBuffer(buffer);
    }

    s_commandBufferPool.deallocateBuffer(buffer);
}

}; //namespace GFX
}; //namespace Divide