#include "Headers/GUISplash.h"

#include "Core/Headers/ParamHandler.h"
#include "Core/Resources/Headers/ResourceCache.h"
#include "Geometry/Shapes/Headers/Predefined/Quad3D.h"
#include "Hardware/Video/Headers/GFXDevice.h"

GUISplash::GUISplash(const std::string& splashImageName,const vec2<U16>& dimensions)
{
    ResourceDescriptor mrt("SplashScreen RenderQuad");
    mrt.setFlag(true); //No default Material;
    _renderQuad = CreateResource<Quad3D>(mrt);
    assert(_renderQuad);
    _renderQuad->setDimensions(vec4<F32>(0,0,dimensions.width,dimensions.height));
    SamplerDescriptor splashSampler;
    splashSampler.toggleMipMaps(true);
    splashSampler.setAnisotropy(16);
    splashSampler.setWrapMode(TEXTURE_CLAMP);
    ResourceDescriptor splashImage("SplashScreen Texture");
    splashImage.setFlag(true);
    splashImage.setThreadedLoading(false);
    splashImage.setPropertyDescriptor<SamplerDescriptor>(splashSampler);
    std::string splashImageLocation = ParamHandler::getInstance().getParam<std::string>("assetsLocation") + "/misc_images/";
    splashImageLocation += splashImageName;
    splashImage.setResourceLocation(splashImageLocation);

    _splashImage = CreateResource<Texture>(splashImage);
    ResourceDescriptor splashShader("fboPreview");
    splashShader.setThreadedLoading(false);
    _splashShader = CreateResource<ShaderProgram>(splashShader);
    _renderQuad->setCustomShader(_splashShader);
    _renderQuad->renderInstance()->preDraw(true);
    _renderQuad->renderInstance()->draw2D(true);
}

GUISplash::~GUISplash()
{
    RemoveResource(_renderQuad);
    RemoveResource(_splashImage);
    RemoveResource(_splashShader);
}

void GUISplash::render(){
    GFX_DEVICE.toggle2D(true);
        _splashShader->bind();
        _splashShader->UniformTexture("tex",0);
        _splashImage->Bind(0);
        GFX_DEVICE.renderInstance(_renderQuad->renderInstance());
        _splashImage->Unbind(0);
    GFX_DEVICE.toggle2D(false);
}