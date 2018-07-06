#include "Core/Resources/Headers/ResourceLoader.h"
#include "Core/Resources/Headers/ResourceCache.h"
#include "Environment/Sky/Headers/Sky.h"

namespace Divide {

template<>
Resource_ptr ImplResourceLoader<Sky>::operator()() {
    std::shared_ptr<Sky> ptr(MemoryManager_NEW Sky(_descriptor.getName(), _descriptor.getID()), DeleteResource());

    if (!load(ptr)) {
        ptr.reset();
    }

    return ptr;
}

};
