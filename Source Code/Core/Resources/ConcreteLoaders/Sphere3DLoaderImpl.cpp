#include "stdafx.h"

#include "Core/Headers/PlatformContext.h"
#include "Core/Resources/Headers/ResourceLoader.h"
#include "Core/Resources/Headers/ResourceCache.h"
#include "Geometry/Material/Headers/Material.h"
#include "Geometry/Shapes/Predefined/Headers/Sphere3D.h"

namespace Divide {

CachedResource_ptr ImplResourceLoader<Sphere3D>::operator()() {
    std::shared_ptr<Sphere3D> ptr(MemoryManager_NEW Sphere3D(_context.gfx(),
                                                               _cache,
                                                               _loadingDescriptorHash,
                                                               _descriptor.resourceName(),
                                                               _descriptor.enumValue() == 0
                                                                                        ? 1.0f
                                                                                        : to_F32(_descriptor.enumValue()),
                                                               _descriptor.ID() == 0 
                                                                                 ? 32 
                                                                                 : _descriptor.ID()),
                                    DeleteResource(_cache));

    if (!_descriptor.flag()) {
        const ResourceDescriptor matDesc("Material_" + _descriptor.resourceName());
        Material_ptr matTemp = CreateResource<Material>(_cache, matDesc);
        matTemp->shadingMode(ShadingMode::BLINN_PHONG);
        ptr->setMaterialTpl(matTemp);
    }

    if (!Load(ptr)) {
        ptr.reset();
    }

    return ptr;
}

}
