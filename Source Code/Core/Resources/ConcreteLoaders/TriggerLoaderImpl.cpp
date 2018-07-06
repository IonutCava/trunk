#include "Core/Resources/Headers/ResourceLoader.h"
#include "Core/Resources/Headers/ResourceCache.h"
#include "Dynamics/Entities/Triggers/Headers/Trigger.h"

namespace Divide {

template<>
Trigger* ImplResourceLoader<Trigger>::operator()() {
    Trigger* ptr = MemoryManager_NEW Trigger(_descriptor.getName());

    if (!load(ptr)) {
        MemoryManager::DELETE(ptr);
    } else {
        ptr->renderState().useDefaultMaterial(false);
    }

    return ptr;
}

};
