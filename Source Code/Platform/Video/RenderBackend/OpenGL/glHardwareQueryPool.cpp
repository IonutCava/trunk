#include "stdafx.h"

#include "Headers/glHardwareQueryPool.h"

namespace Divide {

glHardwareQueryPool::glHardwareQueryPool(GFXDevice& context)
    : _context(context),
      _index(0)
{
}

glHardwareQueryPool::~glHardwareQueryPool()
{
    destroy();
}

void glHardwareQueryPool::init(U32 size) {
    destroy();
    const U32 j = std::max(size, 1u);
    for (U32 i = 0; i < j; ++i) {
        _queryPool.emplace_back(MemoryManager_NEW glHardwareQueryRing(_context, 1, i));
    }
}

void glHardwareQueryPool::destroy() {
    MemoryManager::DELETE_CONTAINER(_queryPool);
}

glHardwareQueryRing& glHardwareQueryPool::allocate() {
    return *_queryPool[++_index];
}

void glHardwareQueryPool::deallocate(glHardwareQueryRing& query) {
    for (U32 i = 0; i < _index; ++i) {
        if (_queryPool[i]->id() == query.id()) {
            std::swap(_queryPool[i], _queryPool[_index - 1]);
            --_index;
            return;
        }
    }

    DIVIDE_UNEXPECTED_CALL("Target deallocation query not part of the pool!");
}

}; //namespace Divide