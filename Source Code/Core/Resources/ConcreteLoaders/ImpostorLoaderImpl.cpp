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
    eastl::shared_ptr<ImpostorSphere> ptr(MemoryManager_NEW ImpostorSphere(_context.gfx(), _cache, _loadingDescriptorHash, _descriptor.resourceName(), 1.0f),
                                          DeleteResource(_cache));

    if (!_descriptor.flag()) {
        Material_ptr matTemp = CreateResource<Material>(_cache, ResourceDescriptor("Material_" + _descriptor.resourceName()));

        RenderStateBlock dummyDesc(RenderStateBlock::get(matTemp->getRenderStateBlock({ RenderStage::DISPLAY, RenderPassType::MAIN_PASS })));
        dummyDesc.setFillMode(FillMode::WIREFRAME);
        matTemp->setRenderStateBlock(dummyDesc.getHash(), { RenderStage::DISPLAY, RenderPassType::MAIN_PASS });
        matTemp->setRenderStateBlock(dummyDesc.getHash(), { RenderStage::DISPLAY, RenderPassType::OIT_PASS });
        matTemp->setShadingMode(ShadingMode::FLAT);

        ptr->setMaterialTpl(matTemp);
    }

    if (!load(ptr)) {
        ptr.reset();
    }

    return ptr;
}

template <>
CachedResource_ptr ImplResourceLoader<ImpostorBox>::operator()() {
    eastl::shared_ptr<ImpostorBox> ptr(MemoryManager_NEW ImpostorBox(_context.gfx(), _cache, _loadingDescriptorHash, _descriptor.resourceName(), 1.0f),
                                       DeleteResource(_cache));

    if (!_descriptor.flag()) {
        Material_ptr matTemp = CreateResource<Material>(_cache, ResourceDescriptor("Material_" + _descriptor.resourceName()));

        RenderStateBlock dummyDesc(RenderStateBlock::get(matTemp->getRenderStateBlock({ RenderStage::DISPLAY, RenderPassType::MAIN_PASS })));
        dummyDesc.setFillMode(FillMode::WIREFRAME);
        matTemp->setRenderStateBlock(dummyDesc.getHash(), {RenderStage::DISPLAY, RenderPassType::MAIN_PASS});
        matTemp->setRenderStateBlock(dummyDesc.getHash(), {RenderStage::DISPLAY, RenderPassType::OIT_PASS});
        matTemp->setShadingMode(ShadingMode::FLAT);

        ptr->setMaterialTpl(matTemp);
    }

    if (!load(ptr)) {
        ptr.reset();
    }

    return ptr;
}

};
