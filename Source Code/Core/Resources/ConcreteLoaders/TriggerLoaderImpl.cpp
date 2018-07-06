#include "stdafx.h"

#include "Core/Resources/Headers/ResourceLoader.h"
#include "Core/Resources/Headers/ResourceCache.h"
#include "Dynamics/Entities/Triggers/Headers/Trigger.h"

namespace Divide {

template<>
CachedResource_ptr ImplResourceLoader<Trigger>::operator()() {
    std::shared_ptr<Trigger> ptr(MemoryManager_NEW Trigger(_cache, _loadingDescriptorHash, _descriptor.name()),
                                 DeleteResource(_cache));

    if (!load(ptr, _descriptor.onLoadCallback())) {
        ptr.reset();
    }

    return ptr;
}

};
