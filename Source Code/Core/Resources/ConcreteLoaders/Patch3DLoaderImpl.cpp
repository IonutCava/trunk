#include "stdafx.h"

#include "Core/Headers/PlatformContext.h"
#include "Core/Resources/Headers/ResourceLoader.h"
#include "Core/Resources/Headers/ResourceCache.h"
#include "Geometry/Shapes/Predefined/Headers/Patch3D.h"

namespace Divide {

    template<>
    CachedResource_ptr ImplResourceLoader<Patch3D>::operator()() {
        std::shared_ptr<Patch3D> ptr(MemoryManager_NEW Patch3D(_context.gfx(),
                                                               _cache,
                                                               _loadingDescriptorHash,
                                                               _descriptor.name(),
                                                               *reinterpret_cast<vec2<U16>*>(_descriptor.getUserPtr())),
                                     DeleteResource(_cache));

        if (!load(ptr, _descriptor.onLoadCallback())) {
            ptr.reset();
        }

        return ptr;
    }

};
