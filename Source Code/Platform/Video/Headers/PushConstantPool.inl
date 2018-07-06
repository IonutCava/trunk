#ifndef _PUSH_BUFFER_POOL_INL_
#define _PUSH_BUFFER_POOL_INL_

namespace Divide {
namespace GFX {

    template <typename... Args>
    PushConstant* PushConstantPool::allocateConstant(Args&&... args) {
        WriteLock lock(_mutex);
        return _pool.newElement(std::forward<Args>(args)...);
        //return MemoryManager_NEW PushConstant(std::forward<Args>(args)...);
    }

    template <typename... Args>
    PushConstant* allocatePushConstant(Args&&... args) {
        return s_pushConstantPool.allocateConstant(std::forward<Args>(args)...);
    }
}; //namespace GFX
}; //namespace Divide

#endif //_PUSH_BUFFER_POOL_INL_