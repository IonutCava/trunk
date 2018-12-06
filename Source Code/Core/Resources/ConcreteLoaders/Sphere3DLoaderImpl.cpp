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
                                                             _descriptor.getEnumValue() == 0
                                                                                         ? 1.0f
                                                                                         : to_F32(_descriptor.getEnumValue()),
                                                             _descriptor.getID() == 0 
                                                                                  ? 32 
                                                                                  : _descriptor.getID()),
                                  DeleteResource(_cache));

    if (!_descriptor.getFlag()) {
        ResourceDescriptor matDesc("Material_" + _descriptor.resourceName());
        Material_ptr matTemp = CreateResource<Material>(_cache, matDesc);
        matTemp->setShadingMode(Material::ShadingMode::BLINN_PHONG);
        ptr->setMaterialTpl(matTemp);
    }

    if (!load(ptr, _descriptor.onLoadCallback())) {
        ptr.reset();
    }

    return ptr;
}

};
