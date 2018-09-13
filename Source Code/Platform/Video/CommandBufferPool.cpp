#include "stdafx.h"

#include "Headers/CommandBufferPool.h"

namespace Divide {
namespace GFX {

CommandBufferPool::CommandBufferPool()
{
}

CommandBufferPool::~CommandBufferPool()
{

}

CommandBuffer* CommandBufferPool::allocateBuffer() {
    UniqueLock lock(_mutex);
    return _pool.newElement();
}

void CommandBufferPool::deallocateBuffer(CommandBuffer*& buffer) {
    if (buffer != nullptr) {
        UniqueLock lock(_mutex);
        _pool.deleteElement(buffer);
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