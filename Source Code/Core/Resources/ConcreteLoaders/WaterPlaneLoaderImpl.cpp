#include "stdafx.h"

#include "Core/Resources/Headers/ResourceCache.h"
#include "Core/Headers/StringHelper.h"
#include "Environment/Water/Headers/Water.h"
#include "Platform/File/Headers/FileManagement.h"
#include "Platform/Video/Headers/GFXDevice.h"
#include "Platform/Video/Headers/RenderStateBlock.h"
#include "Geometry/Material/Headers/Material.h"

namespace Divide {

template <>
bool ImplResourceLoader<WaterPlane>::load(std::shared_ptr<WaterPlane> res, const DELEGATE_CBK<void, CachedResource_wptr>& onLoadCallback) {
    const stringImpl& name = res->name();

    res->setState(ResourceState::RES_LOADING);

    SamplerDescriptor defaultSampler;
    defaultSampler.setWrapMode(TextureWrap::REPEAT);
    defaultSampler.setMinFilter(TextureFilter::LINEAR);

    ResourceDescriptor waterShader("water");
    ResourceDescriptor waterMaterial("waterMaterial_" + name);
    ResourceDescriptor waterTexture("waterTexture_" + name);
    ResourceDescriptor waterTextureDUDV("waterTextureDUDV_" + name);

    TextureDescriptor texDescriptor(TextureType::TEXTURE_2D);
    texDescriptor.setSampler(defaultSampler);

    waterTexture.setResourceName("terrain_water_NM.jpg");
    waterTexture.setResourceLocation(Paths::g_assetsLocation + Paths::g_imagesLocation);
    waterTexture.setPropertyDescriptor(texDescriptor);

    waterTextureDUDV.setResourceName("water_dudv.jpg");
    waterTextureDUDV.setResourceLocation(Paths::g_assetsLocation + Paths::g_imagesLocation);
    waterTextureDUDV.setPropertyDescriptor(texDescriptor);

    Texture_ptr waterNM = CreateResource<Texture>(_cache, waterTexture);
    assert(waterNM != nullptr);

    Texture_ptr waterDUDV = CreateResource<Texture>(_cache, waterTextureDUDV);
    assert(waterDUDV != nullptr);

    ShaderProgram_ptr waterShaderProgram = CreateResource<ShaderProgram>(_cache, waterShader);
    assert(waterShaderProgram != nullptr);

    Material_ptr waterMat = CreateResource<Material>(_cache, waterMaterial);
    assert(waterMat != nullptr);
    // The material is responsible for the destruction of the textures and
    // shaders it receives!!!!
    res->setMaterialTpl(waterMat);

    waterMat->dumpToFile(false);
    waterMat->setShadingMode(Material::ShadingMode::BLINN_PHONG);
    waterMat->setTexture(ShaderProgram::TextureUsage::UNIT0, waterNM);
    waterMat->setTexture(ShaderProgram::TextureUsage::UNIT1, waterDUDV);
    waterMat->setShaderProgram(waterShaderProgram->name(), true);
    waterMat->setShaderProgram("depthPass.PrePass", RenderPassType::DEPTH_PASS, true);

    size_t hash = waterMat->getRenderStateBlock(RenderStagePass(RenderStage::DISPLAY, RenderPassType::COLOUR_PASS));
    RenderStateBlock waterMatDesc(RenderStateBlock::get(hash));
    waterMatDesc.setCullMode(CullMode::NONE);
    waterMat->setRenderStateBlock(waterMatDesc.getHash());

    return res->load(onLoadCallback);
}

template<>
CachedResource_ptr ImplResourceLoader<WaterPlane>::operator()() {
    vec3<F32> dimensions = *reinterpret_cast<vec3<F32>*>(_descriptor.getUserPtr());

    std::shared_ptr<WaterPlane> ptr(MemoryManager_NEW WaterPlane(_cache, _loadingDescriptorHash, _descriptor.name(), dimensions),
                                    DeleteResource(_cache));

    if (!load(ptr, _descriptor.onLoadCallback())) {
        ptr.reset();
    }

    return ptr;
}

};
