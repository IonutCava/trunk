#include "Headers/GUISplash.h"

#include "Core/Headers/ParamHandler.h"
#include "Core/Resources/Headers/ResourceCache.h"
#include "Geometry/Shapes/Headers/Predefined/Quad3D.h"
#include "Hardware/Video/Headers/GFXDevice.h"

GUISplash::GUISplash(const std::string& splashImageName,const vec2<U16>& dimensions)
{
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
    ResourceDescriptor splashShader("fbPreview");
    splashShader.setThreadedLoading(false);
    _splashShader = CreateResource<ShaderProgram>(splashShader);
    _splashShader->UniformTexture("tex", 0);
}

GUISplash::~GUISplash()
{
    RemoveResource(_splashImage);
    RemoveResource(_splashShader);
}

void GUISplash::render(){
    _splashShader->bind();
    _splashImage->Bind(0);
    GFX_DEVICE.drawPoints(1);
}