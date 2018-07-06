#include "Core/Resources/Headers/ResourceLoader.h"
#include "Core/Resources/Headers/ResourceCache.h"
#include "Environment/Sky/Headers/Sky.h"

namespace Divide {

template<>
Sky* ImplResourceLoader<Sky>::operator()() {
    Sky* ptr = MemoryManager_NEW Sky(_descriptor.getName());

    if (!load(ptr)) {
        MemoryManager::DELETE(ptr);
    }

    return ptr;
}

};
