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

#pragma message("HACK: ToDo: add proper water alpha controls")
template<>
bool ImplResourceLoader<WaterPlane>::load(WaterPlane* const res, const std::string& name) {
    res->setState(RES_LOADING);

    ///Set water plane to be single-sided
    P32 quadMask;
    quadMask.i = 0;
    quadMask.b.b0 = true;

    SamplerDescriptor defaultSampler;
    defaultSampler.setWrapMode(TEXTURE_REPEAT);
    defaultSampler.toggleMipMaps(false);
    ResourceDescriptor waterMaterial("waterMaterial");
    ResourceDescriptor waterShader("water");
    ResourceDescriptor waterPlane("waterPlane");
    ResourceDescriptor waterTexture("waterTexture");
    waterTexture.setResourceLocation(ParamHandler::getInstance().getParam<std::string>("assetsLocation")+"/misc_images/terrain_water_NM.jpg");
    waterTexture.setPropertyDescriptor(defaultSampler);
    waterPlane.setFlag(true); //No default material
    waterPlane.setBoolMask(quadMask);
    Texture2D* waterNM = CreateResource<Texture2D>(waterTexture);
    assert(waterNM != NULL);

    ShaderProgram* waterShaderProgram = CreateResource<ShaderProgram>(waterShader);
    assert(waterShaderProgram != NULL);

    Material* waterMat = CreateResource<Material>(waterMaterial);
    assert(waterMat != NULL);
    //The material is responsible for the destruction of the textures and shaders it receives!!!!
    res->setMaterial(waterMat);
    res->setWaterNormalMap(waterNM);
    res->setShaderProgram(waterShaderProgram);
    res->setGeometry(CreateResource<Quad3D>(waterPlane));

    waterMat->setTexture(Material::TEXTURE_UNIT0, waterNM);
    waterMat->setShaderProgram(waterShaderProgram->getName());
    vec3<F32> waterDiffuse = waterMat->getMaterialMatrix().getCol(1);
    waterMat->setDiffuse(vec4<F32>(waterDiffuse, 0.5f));

    RenderStateBlockDescriptor& waterMatDesc = waterMat->getRenderState(FINAL_STAGE)->getDescriptor();
    waterMatDesc.setCullMode(CULL_MODE_NONE);
    waterMatDesc.setBlend(true, BLEND_PROPERTY_SRC_ALPHA, BLEND_PROPERTY_INV_SRC_ALPHA);

    return true;
}