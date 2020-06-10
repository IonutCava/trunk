#include "stdafx.h"

#include "Core/Headers/PlatformContext.h"
#include "Core/Resources/Headers/ResourceLoader.h"
#include "Core/Resources/Headers/ResourceCache.h"
#include "Geometry/Shapes/Headers/SkinnedSubMesh.h"

namespace Divide {

template<>
CachedResource_ptr ImplResourceLoader<SubMesh>::operator()() {
    SubMesh_ptr ptr;

    if (_descriptor.enumValue() == to_base(Object3D::ObjectFlag::OBJECT_FLAG_SKINNED)) {
        ptr.reset(MemoryManager_NEW SkinnedSubMesh(_context.gfx(), _cache, _loadingDescriptorHash, _descriptor.resourceName()), DeleteResource(_cache));
    } else {
        ptr.reset(MemoryManager_NEW SubMesh(_context.gfx(), _cache, _loadingDescriptorHash, _descriptor.resourceName()), DeleteResource(_cache));
    }

    if (!Load(ptr)) {
        ptr.reset();
    } else {
        ptr->setID(_descriptor.ID());
    }

    return ptr;
}

};
