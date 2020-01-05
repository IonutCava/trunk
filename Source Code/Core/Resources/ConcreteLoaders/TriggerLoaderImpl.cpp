#include "stdafx.h"

#include "Core/Resources/Headers/ResourceLoader.h"
#include "Core/Resources/Headers/ResourceCache.h"
#include "Dynamics/Entities/Triggers/Headers/Trigger.h"

namespace Divide {

template<>
CachedResource_ptr ImplResourceLoader<Trigger>::operator()() {
    eastl::shared_ptr<Trigger> ptr(MemoryManager_NEW Trigger(_cache, _loadingDescriptorHash, _descriptor.resourceName()),
                                   DeleteResource(_cache));

    if (!load(ptr)) {
        ptr.reset();
    }

    return ptr;
}

};
