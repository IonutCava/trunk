#include "Core/Resources/Headers/ResourceLoader.h"
#include "Core/Resources/Headers/ResourceCache.h"
#include "Dynamics/Entities/Triggers/Headers/Trigger.h"

namespace Divide {

template<>
Trigger* ImplResourceLoader<Trigger>::operator()() {
    Trigger* ptr = MemoryManager_NEW Trigger();

    if (!load(ptr, _descriptor.getName())) {
        MemoryManager::DELETE(ptr);
    } else {
        ptr->renderState().useDefaultMaterial(false);
    }

    return ptr;
}

};
