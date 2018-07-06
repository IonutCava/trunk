#include "Core/Resources/Headers/ResourceLoader.h"
#include "Core/Resources/Headers/ResourceCache.h"
#include "Dynamics/Entities/Headers/Impostor.h"
#include "Geometry/Material/Headers/Material.h"

namespace Divide {

template <>
ImpostorSphere* ImplResourceLoader<ImpostorSphere>::operator()() {
    ImpostorSphere* ptr = MemoryManager_NEW ImpostorSphere(_descriptor.getName(), 1.0f);

    if (_descriptor.getFlag()) {
        ptr->renderState().useDefaultMaterial(false);
    } else {
        Material* matTemp = CreateResource<Material>(ResourceDescriptor("Material_" + _descriptor.getName()));
        RenderStateBlock dummyDesc(GFX_DEVICE.getRenderStateBlock(matTemp->getRenderStateBlock(RenderStage::DISPLAY)));
        dummyDesc.setFillMode(FillMode::WIREFRAME);
        matTemp->setRenderStateBlock(dummyDesc.getHash(), RenderStage::DISPLAY);
        matTemp->setShadingMode(Material::ShadingMode::FLAT);

        ptr->setMaterialTpl(matTemp);
    }

    if (!load(ptr)) {
        MemoryManager::DELETE(ptr);
    }

    return ptr;
}


template <>
ImpostorBox* ImplResourceLoader<ImpostorBox>::operator()() {
    ImpostorBox* ptr = MemoryManager_NEW ImpostorBox(_descriptor.getName(), 1.0f);

    if (_descriptor.getFlag()) {
        ptr->renderState().useDefaultMaterial(false);
    } else {
        Material* matTemp = CreateResource<Material>(ResourceDescriptor("Material_" + _descriptor.getName()));
        RenderStateBlock dummyDesc(GFX_DEVICE.getRenderStateBlock(matTemp->getRenderStateBlock(RenderStage::DISPLAY)));
        dummyDesc.setFillMode(FillMode::WIREFRAME);
        matTemp->setRenderStateBlock(dummyDesc.getHash(), RenderStage::DISPLAY);
        matTemp->setShadingMode(Material::ShadingMode::FLAT);

        ptr->setMaterialTpl(matTemp);
    }

    if (!load(ptr)) {
        MemoryManager::DELETE(ptr);
    }

    return ptr;
}

};
