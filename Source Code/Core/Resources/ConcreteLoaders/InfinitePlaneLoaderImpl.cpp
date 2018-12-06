#include "stdafx.h"

#include "Core/Headers/PlatformContext.h"
#include "Core/Resources/Headers/ResourceLoader.h"
#include "Core/Resources/Headers/ResourceCache.h"
#include "Environment/Terrain/Headers/InfinitePlane.h"

namespace Divide {

    template<>
    CachedResource_ptr ImplResourceLoader<InfinitePlane>::operator()() {
        std::shared_ptr<InfinitePlane> ptr(MemoryManager_NEW InfinitePlane(_context.gfx(), _cache, _loadingDescriptorHash, _descriptor.resourceName(), vec2<U16>(_descriptor.getID())),
            DeleteResource(_cache));

        if (!load(ptr, _descriptor.onLoadCallback())) {
            ptr.reset();
        }

        return ptr;
    }

};
