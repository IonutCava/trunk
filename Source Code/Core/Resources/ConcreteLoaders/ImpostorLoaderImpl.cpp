#include "stdafx.h"

#include "Core/Headers/PlatformContext.h"
#include "Core/Resources/Headers/ResourceLoader.h"
#include "Core/Resources/Headers/ResourceCache.h"
#include "Dynamics/Entities/Headers/Impostor.h"
#include "Geometry/Material/Headers/Material.h"
#include "Platform/Video/Headers/RenderStateBlock.h"

namespace Divide {

template <>
CachedResource_ptr ImplResourceLoader<ImpostorSphere>::operator()() {
    std::shared_ptr<ImpostorSphere> ptr(MemoryManager_NEW ImpostorSphere(_context.gfx(), _cache, _loadingDescriptorHash, _descriptor.name(), 1.0f),
                                        DeleteResource(_cache));

    if (_descriptor.getFlag()) {
        ptr->renderState().useDefaultMaterial(false);
    } else {
        Material_ptr matTemp = 
            CreateResource<Material>(_cache, ResourceDescriptor("Material_" + _descriptor.name()));

        RenderStateBlock dummyDesc(RenderStateBlock::get(matTemp->getRenderStateBlock(RenderStagePass(RenderStage::DISPLAY, RenderPassType::COLOUR_PASS))));
        dummyDesc.setFillMode(FillMode::WIREFRAME);
        matTemp->setRenderStateBlock(dummyDesc.getHash(), RenderStagePass(RenderStage::DISPLAY, RenderPassType::COLOUR_PASS));
        matTemp->setShadingMode(Material::ShadingMode::FLAT);

        ptr->setMaterialTpl(matTemp);
    }

    if (!load(ptr, _descriptor.onLoadCallback())) {
        ptr.reset();
    }

    return ptr;
}

template <>
CachedResource_ptr ImplResourceLoader<ImpostorBox>::operator()() {
    std::shared_ptr<ImpostorBox> ptr(MemoryManager_NEW ImpostorBox(_context.gfx(), _cache, _loadingDescriptorHash, _descriptor.name(), 1.0f),
                                     DeleteResource(_cache));

    if (_descriptor.getFlag()) {
        ptr->renderState().useDefaultMaterial(false);
    } else {
        Material_ptr matTemp =
            CreateResource<Material>(_cache, ResourceDescriptor("Material_" + _descriptor.name()));

        RenderStateBlock dummyDesc(RenderStateBlock::get(matTemp->getRenderStateBlock(RenderStagePass(RenderStage::DISPLAY, RenderPassType::COLOUR_PASS))));
        dummyDesc.setFillMode(FillMode::WIREFRAME);
        matTemp->setRenderStateBlock(dummyDesc.getHash(), RenderStagePass(RenderStage::DISPLAY, RenderPassType::COLOUR_PASS));
        matTemp->setShadingMode(Material::ShadingMode::FLAT);

        ptr->setMaterialTpl(matTemp);
    }

    if (!load(ptr, _descriptor.onLoadCallback())) {
        ptr.reset();
    }

    return ptr;
}

};
