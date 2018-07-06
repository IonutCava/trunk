#include "Core/Resources/Headers/ResourceLoader.h"
#include "Core/Resources/Headers/ResourceCache.h"
#include "Dynamics/Entities/Headers/Impostor.h"
#include "Geometry/Material/Headers/Material.h"

namespace Divide {

template <>
Resource_ptr ImplResourceLoader<ImpostorSphere>::operator()() {
    std::shared_ptr<ImpostorSphere> ptr(MemoryManager_NEW ImpostorSphere(_descriptor.getName(), 1.0f), DeleteResource());

    if (_descriptor.getFlag()) {
        ptr->renderState().useDefaultMaterial(false);
    } else {
        Material_ptr matTemp = 
            CreateResource<Material>(ResourceDescriptor("Material_" + _descriptor.getName()));

        RenderStateBlock dummyDesc(GFX_DEVICE.getRenderStateBlock(matTemp->getRenderStateBlock(RenderStage::DISPLAY)));
        dummyDesc.setFillMode(FillMode::WIREFRAME);
        matTemp->setRenderStateBlock(dummyDesc.getHash(), RenderStage::DISPLAY);
        matTemp->setShadingMode(Material::ShadingMode::FLAT);

        ptr->setMaterialTpl(matTemp);
    }

    if (!load(ptr)) {
        ptr.reset();
    }

    return ptr;
}


template <>
Resource_ptr ImplResourceLoader<ImpostorBox>::operator()() {
    std::shared_ptr<ImpostorBox> ptr(MemoryManager_NEW ImpostorBox(_descriptor.getName(), 1.0f), DeleteResource());

    if (_descriptor.getFlag()) {
        ptr->renderState().useDefaultMaterial(false);
    } else {
        Material_ptr matTemp =
            CreateResource<Material>(ResourceDescriptor("Material_" + _descriptor.getName()));

        RenderStateBlock dummyDesc(GFX_DEVICE.getRenderStateBlock(matTemp->getRenderStateBlock(RenderStage::DISPLAY)));
        dummyDesc.setFillMode(FillMode::WIREFRAME);
        matTemp->setRenderStateBlock(dummyDesc.getHash(), RenderStage::DISPLAY);
        matTemp->setShadingMode(Material::ShadingMode::FLAT);

        ptr->setMaterialTpl(matTemp);
    }

    if (!load(ptr)) {
        ptr.reset();
    }

    return ptr;
}

};
