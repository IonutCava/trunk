#include "Headers/GUISplash.h"

#include "Core/Headers/ParamHandler.h"
#include "Core/Resources/Headers/ResourceCache.h"
#include "Geometry/Shapes/Headers/Predefined/Quad3D.h"
#include "Platform/Video/Headers/GFXDevice.h"

namespace Divide {

GUISplash::GUISplash(const stringImpl& splashImageName,
                     const vec2<U16>& dimensions) 
    : _dimensions(dimensions)
{
    SamplerDescriptor splashSampler;
    splashSampler.toggleMipMaps(false);
    splashSampler.setAnisotropy(0);
    splashSampler.setWrapMode(TextureWrap::CLAMP);
    ResourceDescriptor splashImage("SplashScreen Texture");
    splashImage.setThreadedLoading(false);
    splashImage.setPropertyDescriptor<SamplerDescriptor>(splashSampler);
    stringImpl splashImageLocation = stringAlg::toBase(
        ParamHandler::getInstance().getParam<std::string>("assetsLocation") +
        "/misc_images/");
    splashImageLocation += splashImageName;
    splashImage.setResourceLocation(splashImageLocation);

    _splashImage = CreateResource<Texture>(splashImage);
    ResourceDescriptor splashShader("fbPreview");
    splashShader.setThreadedLoading(false);
    _splashShader = CreateResource<ShaderProgram>(splashShader);
}

GUISplash::~GUISplash()
{
    RemoveResource(_splashImage);
    RemoveResource(_splashShader);
}

void GUISplash::render() {
    GFX::ScopedViewport splashViewport(vec4<I32>(0, 0, _dimensions.width, _dimensions.height));
    _splashImage->Bind(static_cast<U8>(ShaderProgram::TextureUsage::UNIT0));
    GFX_DEVICE.drawPoints(1, GFX_DEVICE.getDefaultStateBlock(true),
                          _splashShader);
}
};