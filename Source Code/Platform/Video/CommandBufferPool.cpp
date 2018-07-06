#include "stdafx.h"

#include "Headers/CommandBufferPool.h"

namespace Divide {
namespace GFX {

CommandBufferPool::CommandBufferPool()
{
    _bufferPoolSlotState.reserve(Config::COMMAND_BUFFER_POOL_SIZE);
    _bufferPool.reserve(Config::COMMAND_BUFFER_POOL_SIZE);
}

CommandBufferPool::~CommandBufferPool()
{

}

CommandBuffer& CommandBufferPool::allocateBuffer() {
    UpgradableReadLock ur_lock(_mutex);
    vectorAlg::vecSize count = _bufferPoolSlotState.size();
    for (size_t i = 0; i < count; ++i) {
        if (!_bufferPoolSlotState[i]) {
            UpgradeToWriteLock w_lock(ur_lock);
            CommandBuffer& ret = _bufferPool[i];
            ret._poolEntryIndex = i;
            _bufferPoolSlotState[i] = true;
            return ret;
        }
    }

    _bufferPoolSlotState.push_back(true);
    CommandBuffer buff;
    buff._poolEntryIndex = count;
    _bufferPool.push_back(buff);

    return _bufferPool.back();
}

void CommandBufferPool::deallocateBuffer(CommandBuffer& buffer) {
    size_t entryIndex = buffer._poolEntryIndex;
    buffer.clear();
    buffer._poolEntryIndex = std::numeric_limits<size_t>::max();

    WriteLock w_lock(_mutex);
    // Attempt to recover some memory if this pool overflowed.
    // Don't erase mid entries (even if they're over N) as that would mess up 
    // entry indices in flight
    if (entryIndex == _bufferPoolSlotState.size() - 1) {
        _bufferPool.pop_back();
        _bufferPoolSlotState.pop_back();
    } else {
        _bufferPoolSlotState[entryIndex] = false;
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

CommandBuffer& allocateCommandBuffer(bool useSecondaryBuffers) {
    if (useSecondaryBuffers) {
        return s_secondaryCommandBufferPool.allocateBuffer();
    }
    return s_commandBufferPool.allocateBuffer();
}

void deallocateCommandBuffer(GFX::CommandBuffer& buffer, bool useSecondaryBuffers) {
    if (useSecondaryBuffers) {
        return s_secondaryCommandBufferPool.deallocateBuffer(buffer);
    }

    s_commandBufferPool.deallocateBuffer(buffer);
}

}; //namespace GFX
}; //namespace Divide