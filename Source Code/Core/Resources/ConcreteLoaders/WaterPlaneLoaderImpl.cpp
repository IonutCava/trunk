#include "Core/Resources/Headers/ResourceCache.h"
#include "Core/Headers/ParamHandler.h"
#include "Environment/Water/Headers/Water.h"
#include "Hardware/Video/Headers/GFXDevice.h"
#include "Geometry/Material/Headers/Material.h"

namespace Divide {

WaterPlane* ImplResourceLoader<WaterPlane>::operator()(){
    WaterPlane* ptr = MemoryManager_NEW WaterPlane();

    if(!load(ptr, _descriptor.getName())){
        MemoryManager::DELETE( ptr );
    }

    return ptr;
}

template<>
bool ImplResourceLoader<WaterPlane>::load(WaterPlane* const res, const stringImpl& name) {
    ParamHandler& param = ParamHandler::getInstance();
    res->setState(RES_LOADING);

    SamplerDescriptor defaultSampler;
    defaultSampler.setWrapMode(TEXTURE_REPEAT);
    defaultSampler.toggleMipMaps(false);
    ResourceDescriptor waterMaterial("waterMaterial_"+name);
    ResourceDescriptor waterShader( "water_" + name );
    ResourceDescriptor waterTexture( "waterTexture_" + name );
    ResourceDescriptor waterTextureDUDV( "waterTextureDUDV_" + name );
    waterTexture.setResourceLocation(param.getParam<stringImpl>("assetsLocation") + 
                                     "/misc_images/terrain_water_NM.jpg");
    waterTexture.setPropertyDescriptor(defaultSampler);
    waterTextureDUDV.setResourceLocation(param.getParam<stringImpl>("assetsLocation") + 
                                         "/misc_images/water_dudv.jpg");
    waterTextureDUDV.setPropertyDescriptor(defaultSampler);
    
    Texture* waterNM = CreateResource<Texture>(waterTexture);
    assert(waterNM != nullptr);

    ShaderProgram* waterShaderProgram = CreateResource<ShaderProgram>(waterShader);
    waterShaderProgram->UniformTexture( "texWaterNoiseNM", 0 );
    waterShaderProgram->UniformTexture( "texWaterReflection", 1 );
    waterShaderProgram->UniformTexture( "texWaterRefraction", 2 );
    waterShaderProgram->UniformTexture( "texWaterNoiseDUDV", 3 );
    assert(waterShaderProgram != nullptr);

    Material* waterMat = CreateResource<Material>(waterMaterial);
    assert(waterMat != nullptr);
    //The material is responsible for the destruction of the textures and shaders it receives!!!!
    res->setMaterialTpl(waterMat);

    waterMat->dumpToFile(false);
    waterMat->setShadingMode(Material::SHADING_BLINN_PHONG);
    waterMat->setTexture(ShaderProgram::TEXTURE_UNIT0, waterNM);
    waterMat->setShaderProgram(waterShaderProgram->getName(), FINAL_STAGE, true);
    waterMat->setShaderProgram("depthPass.PrePass", Z_PRE_PASS_STAGE, true);
 
    RenderStateBlockDescriptor waterMatDesc(GFX_DEVICE.getStateBlockDescriptor(waterMat->getRenderStateBlock(FINAL_STAGE)));
    waterMatDesc.setCullMode(CULL_MODE_NONE);
    waterMat->setRenderStateBlock(waterMatDesc, FINAL_STAGE);

    return true;
}

};