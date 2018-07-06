#include "stdafx.h"

#include "Headers/PushConstantPool.h"

namespace Divide {
namespace GFX {
    PushConstantPool::PushConstantPool()
        : _count(0)
    {
    }

    PushConstantPool::~PushConstantPool()
    {

    }

    void PushConstantPool::deallocateConstant(PushConstant*& constants) {
        if (constants != nullptr) {
            WriteLock lock(_mutex);
            _pool.deleteElement(constants);
            //MemoryManager::DELETE(constants);
            //constants = nullptr;
        }
    }

    void deallocatePushConstant(PushConstant*& buffer) {
        s_pushConstantPool.deallocateConstant(buffer);
    }

}; //namespace GFX
}; //namespace Divide