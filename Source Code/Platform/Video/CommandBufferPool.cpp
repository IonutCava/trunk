#include "stdafx.h"

#include "Headers/CommandBufferPool.h"

namespace Divide {
namespace GFX {

#define USE_MEMORY_POOL

namespace {
#if !defined(USE_MEMORY_POOL)
    static SharedLock s_mutex;
    static std::deque<CommandBuffer> s_pool;
    static std::vector<bool> s_freeList;
#endif
};

CommandBufferPool::CommandBufferPool()
    : _bufferCount(0)
{
}

CommandBufferPool::~CommandBufferPool()
{

}

CommandBuffer* CommandBufferPool::allocateBuffer() {
#if defined(USE_MEMORY_POOL)
    WriteLock lock(_mutex);
    return _pool.newElement(_bufferCount++);
#else
    WriteLock lock(s_mutex);
    for (size_t i = 0; i < s_freeList.size(); ++i) {
        if (s_freeList[i]) {
            s_freeList[i] = false;
            _bufferCount++;
            return &s_pool[i];
        }
    }

    s_freeList.push_back(false);
    s_pool.emplace_back(s_pool.size());
    s_bufferCount++;
    return &s_pool.back();
#endif
}

void CommandBufferPool::deallocateBuffer(CommandBuffer*& buffer) {
#if defined(USE_MEMORY_POOL)
    WriteLock lock(_mutex);
    _pool.deleteElement(buffer);
#else
    size_t index = buffer->_index;
    buffer->clear();

    WriteLock lock(s_mutex);
    s_freeList[index] = true;
#endif

    buffer = nullptr;
    _bufferCount--;
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