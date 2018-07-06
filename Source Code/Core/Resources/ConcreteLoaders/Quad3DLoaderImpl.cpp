#include "stdafx.h"

#include "Core/Headers/PlatformContext.h"
#include "Core/Resources/Headers/ResourceLoader.h"
#include "Core/Resources/Headers/ResourceCache.h"
#include "Geometry/Shapes/Predefined/Headers/Quad3D.h"

namespace Divide {

template<>
CachedResource_ptr ImplResourceLoader<Quad3D>::operator()() {
    std::shared_ptr<Quad3D> ptr(MemoryManager_NEW Quad3D(_context.gfx(),
                                                         _cache,
                                                         _loadingDescriptorHash,
                                                         _descriptor.name(),
                                                         _descriptor.getMask().b[0] == 0),
                                DeleteResource(_cache));

    if (!load(ptr, _descriptor.onLoadCallback())) {
        ptr.reset();
    } else {
        if (_descriptor.getFlag()) {
            ptr->renderState().useDefaultMaterial(false);
        }
    }

    return ptr;
}

};
