#include "Core/Resources/Headers/ResourceCache.h"
#include "Core/Headers/StringHelper.h"
#include "Environment/Water/Headers/Water.h"
#include "Platform/Video/Headers/GFXDevice.h"
#include "Platform/Video/Headers/RenderStateBlock.h"
#include "Geometry/Material/Headers/Material.h"

namespace Divide {

template <>
bool ImplResourceLoader<WaterPlane>::load(std::shared_ptr<WaterPlane> res) {
    const stringImpl& name = res->getName();

    SamplerDescriptor defaultSampler;
    defaultSampler.setWrapMode(TextureWrap::REPEAT);
    defaultSampler.toggleMipMaps(false);
    ResourceDescriptor waterShader("water");
    ResourceDescriptor waterMaterial("waterMaterial_" + name);
    ResourceDescriptor waterTexture("waterTexture_" + name);
    ResourceDescriptor waterTextureDUDV("waterTextureDUDV_" + name);
    waterTexture.setResourceName("terrain_water_NM.jpg");
    waterTexture.setResourceLocation(Paths::g_assetsLocation + Paths::g_imagesLocation);
    waterTexture.setPropertyDescriptor(defaultSampler);
    waterTextureDUDV.setResourceName("water_dudv.jpg");
    waterTextureDUDV.setResourceLocation(Paths::g_assetsLocation + Paths::g_imagesLocation);
    waterTextureDUDV.setPropertyDescriptor(defaultSampler);

    Texture_ptr waterNM = CreateResource<Texture>(_cache, waterTexture);
    assert(waterNM != nullptr);

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
    waterMat->setShaderProgram(waterShaderProgram->getName(), true);
    waterMat->setShaderProgram("depthPass.PrePass", RenderStage::Z_PRE_PASS, true);

    size_t hash = waterMat->getRenderStateBlock(RenderStage::DISPLAY);
    RenderStateBlock waterMatDesc(RenderStateBlock::get(hash));
    waterMatDesc.setCullMode(CullMode::NONE);
    waterMat->setRenderStateBlock(waterMatDesc.getHash());

    return true;
}

template<>
Resource_ptr ImplResourceLoader<WaterPlane>::operator()() {
    U32 sideLength = _descriptor.getID();
    assert(sideLength > 0 && sideLength < to_uint(std::numeric_limits<I32>::max()));

    std::shared_ptr<WaterPlane> ptr(MemoryManager_NEW WaterPlane(_cache, _descriptor.getName(), to_int(sideLength)),
                                    DeleteResource(_cache));

    if (!load(ptr)) {
        ptr.reset();
    }

    return ptr;
}

};
