#include "Core/Resources/Headers/ResourceCache.h"
#include "Core/Headers/ParamHandler.h"
#include "Environment/Water/Headers/Water.h"
#include "Hardware/Video/Headers/RenderStateBlock.h"
#include "Hardware/Video/Textures/Headers/Texture.h"
#include "Hardware/Video/Shaders/Headers/ShaderProgram.h"
#include "Geometry/Material/Headers/Material.h"

WaterPlane* ImplResourceLoader<WaterPlane>::operator()(){
    WaterPlane* ptr = New WaterPlane();

    if(!load(ptr, _descriptor.getName())){
        SAFE_DELETE(ptr);
    }

    return ptr;
}

template<>
bool ImplResourceLoader<WaterPlane>::load(WaterPlane* const res, const std::string& name) {
    STUBBED("HACK: ToDo: add proper water alpha controls")
    res->setState(RES_LOADING);

    SamplerDescriptor defaultSampler;
    defaultSampler.setWrapMode(TEXTURE_REPEAT);
    defaultSampler.toggleMipMaps(false);
    ResourceDescriptor waterMaterial("waterMaterial");
    ResourceDescriptor waterShader("water");
    ResourceDescriptor waterTexture("waterTexture");
    ResourceDescriptor waterTextureDUDV("waterTextureDUDV");
    waterTexture.setResourceLocation(ParamHandler::getInstance().getParam<std::string>("assetsLocation")+"/misc_images/terrain_water_NM.jpg");
    waterTexture.setPropertyDescriptor(defaultSampler);
    waterTextureDUDV.setResourceLocation(ParamHandler::getInstance().getParam<std::string>("assetsLocation") + "/misc_images/water_dudv.jpg");
    waterTextureDUDV.setPropertyDescriptor(defaultSampler);
    
    Texture* waterNM = CreateResource<Texture>(waterTexture);
    assert(waterNM != nullptr);

    ShaderProgram* waterShaderProgram = CreateResource<ShaderProgram>(waterShader);
    assert(waterShaderProgram != nullptr);

    Material* waterMat = CreateResource<Material>(waterMaterial);
    assert(waterMat != nullptr);
    //The material is responsible for the destruction of the textures and shaders it receives!!!!
    res->setMaterial(waterMat);

    waterMat->dumpToFile(false);
    waterMat->setTexture(Material::TEXTURE_UNIT0, waterNM);
    waterMat->setShaderProgram(waterShaderProgram->getName(), FINAL_STAGE, true);
    waterMat->setShaderProgram("depthPass.PrePass", Z_PRE_PASS_STAGE, true);
 
    RenderStateBlockDescriptor waterMatDesc(waterMat->getRenderStateBlock(FINAL_STAGE).getDescriptor());
    waterMatDesc.setCullMode(CULL_MODE_NONE);
    waterMat->setRenderStateBlock(waterMatDesc, FINAL_STAGE);

    return true;
}