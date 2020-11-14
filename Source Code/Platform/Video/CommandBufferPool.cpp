#include "stdafx.h"

#include "Headers/CommandBufferPool.h"

namespace Divide {
namespace GFX {

static CommandBufferPool s_commandBufferPool;

void initPools() {
    s_commandBufferPool.reset();
}

void destroyPools() {
    s_commandBufferPool.reset();
}

void CommandBufferPool::reset() {
    _pool = {};
}

CommandBuffer* CommandBufferPool::allocateBuffer() {
    return _pool.newElement(_mutex);
}

void CommandBufferPool::deallocateBuffer(CommandBuffer*& buffer) {
    if (buffer != nullptr) {
        _pool.deleteElement(_mutex, buffer);
        buffer = nullptr;
    }
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

void deallocateCommandBuffer(CommandBuffer*& buffer) {
    s_commandBufferPool.deallocateBuffer(buffer);
}

}; //namespace GFX
}; //namespace Divide