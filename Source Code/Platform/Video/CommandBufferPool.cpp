#include "stdafx.h"

#include "Headers/CommandBufferPool.h"

namespace Divide {
namespace GFX {

static CommandBufferPool s_commandBufferPool;
static CommandBufferPool s_secondaryCommandBufferPool;

void initPools() {
    s_commandBufferPool.init();
    s_secondaryCommandBufferPool.init();
}

void destroyPools() {
    s_commandBufferPool.clear();
    s_secondaryCommandBufferPool.clear();
}

CommandBuffer* CommandBufferPool::allocateBuffer() {
    assert(_pool != nullptr);
    
    return _pool->newElement(_mutex);
}

void CommandBufferPool::deallocateBuffer(CommandBuffer*& buffer) {
    assert(_pool != nullptr);

    if (buffer != nullptr) {
        _pool->deleteElement(_mutex, buffer);
        buffer = nullptr;
    }
}

void CommandBufferPool::init() {
    _pool = std::make_unique<MemoryPool<CommandBuffer, 8192 * 2>>();
}

void CommandBufferPool::clear() noexcept {
    _pool.reset();
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