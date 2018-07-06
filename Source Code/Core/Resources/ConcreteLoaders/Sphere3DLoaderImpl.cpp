#include "Core/Resources/Headers/ResourceLoader.h"
#include "Core/Resources/Headers/ResourceCache.h"
#include "Geometry/Material/Headers/Material.h"
#include "Geometry/Shapes/Headers/Predefined/Sphere3D.h"

namespace Divide {

template<>
Sphere3D* ImplResourceLoader<Sphere3D>::operator()() {
    Sphere3D* ptr = MemoryManager_NEW Sphere3D(_descriptor.getName(),
                                               _descriptor.getEnumValue() == 0
                                                                          ? 1.0f
                                                                          : to_float(_descriptor.getEnumValue()),
                                               _descriptor.getID() == 0 
                                                                   ? 32 : 
                                                                   _descriptor.getID());

    if (_descriptor.getFlag()) {
        ptr->renderState().useDefaultMaterial(false);
    } else {
        Material* matTemp = CreateResource<Material>(
            ResourceDescriptor("Material_" + _descriptor.getName()));
        matTemp->setShadingMode(Material::ShadingMode::BLINN_PHONG);
        ptr->setMaterialTpl(matTemp);
    }

    if (!load(ptr)) {
        MemoryManager::DELETE(ptr);
    }

    return ptr;
}

};
