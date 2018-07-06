#include "stdafx.h"

#include "Core/Headers/PlatformContext.h"
#include "Core/Resources/Headers/ResourceLoader.h"
#include "Core/Resources/Headers/ResourceCache.h"
#include "Geometry/Shapes/Predefined/Headers/Box3D.h"

namespace Divide {

template <>
CachedResource_ptr ImplResourceLoader<Box3D>::operator()() {
    D64 size = 1.0;
    if (!_descriptor.getPropertyListString().empty()) {
        size = atof(_descriptor.getPropertyListString().c_str());  //<should work
    }

    std::shared_ptr<Box3D> ptr(MemoryManager_NEW Box3D(_context.gfx(),
                                                       _cache,
                                                       _loadingDescriptorHash,
                                                       _descriptor.getName(),
                                                       vec3<F32>(to_F32(size))),
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
