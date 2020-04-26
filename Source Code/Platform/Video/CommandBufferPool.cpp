#include "stdafx.h"

#include "Headers/CommandBufferPool.h"

namespace Divide {
namespace GFX {

static CommandBufferPool s_commandBufferPool;

void initPools() {
    s_commandBufferPool.init();
}

void destroyPools() {
    s_commandBufferPool.clear();
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

ScopedCommandBuffer::ScopedCommandBuffer()
    : _buffer(allocateCommandBuffer())
{
}

ScopedCommandBuffer::~ScopedCommandBuffer()
{
    deallocateCommandBuffer(_buffer);
}


ScopedCommandBuffer allocateScopedCommandBuffer() {
    OPTICK_EVENT();

    return ScopedCommandBuffer();
}

CommandBuffer* allocateCommandBuffer() {
    return s_commandBufferPool.allocateBuffer();
}

void deallocateCommandBuffer(GFX::CommandBuffer*& buffer) {
    s_commandBufferPool.deallocateBuffer(buffer);
}

}; //namespace GFX
}; //namespace Divide