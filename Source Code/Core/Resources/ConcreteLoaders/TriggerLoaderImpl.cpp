#include "Core/Resources/Headers/ResourceLoader.h"
#include "Core/Resources/Headers/ResourceCache.h"
#include "Dynamics/Entities/Triggers/Headers/Trigger.h"

namespace Divide {

template<>
Resource_ptr ImplResourceLoader<Trigger>::operator()() {
    std::shared_ptr<Trigger> ptr(MemoryManager_NEW Trigger(_descriptor.getName()), DeleteResource());

    if (!load(ptr)) {
        ptr.reset();
    } else {
        ptr->renderState().useDefaultMaterial(false);
    }

    return ptr;
}

};
