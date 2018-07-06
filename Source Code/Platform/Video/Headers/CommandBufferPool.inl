#ifndef _COMMAND_BUFFER_POOL_INL_
#define _COMMAND_BUFFER_POOL_INL_

namespace Divide {
namespace GFX {

template<size_t N>
CommandBufferPool<N>::CommandBufferPool()
{
    _bufferPoolSlotState.fill(false);
}

template<size_t N>
CommandBufferPool<N>::~CommandBufferPool()
{

}

template<size_t N>
CommandBuffer& CommandBufferPool<N>::allocateBuffer() {
    UpgradableReadLock ur_lock(_mutex);
    for (size_t i = 0; i < N; ++i) {
        if (!_bufferPoolSlotState[i]) {
            UpgradeToWriteLock w_lock(ur_lock);
            CommandBuffer& ret = _bufferPool[i];
            ret._poolEntryIndex = i;
            _bufferPoolSlotState[i] = true;
            return ret;
        }
    }

    DIVIDE_ASSERT(false, "CommandBufferPool::allocateBuffer error: No more space to allocate a new buffer in the current pool!");
    _bufferPoolSlotState.back() = true;
    return _bufferPool.back();
}

template<size_t N>
void CommandBufferPool<N>::deallocateBuffer(CommandBuffer& buffer) {
    buffer.clear();
    buffer._poolEntryIndex = std::numeric_limits<size_t>::max();

    WriteLock w_lock(_mutex);
    _bufferPoolSlotState[buffer._poolEntryIndex] = false;
}

}; //namespace GFX
}; //namespace Divide

#endif //_COMMAND_BUFFER_POOL_INL_