#include "stdafx.h"

#include "Headers/GUISplash.h"

#include "Core/Headers/Kernel.h"
#include "Core/Headers/StringHelper.h"
#include "Core/Resources/Headers/ResourceCache.h"
#include "Geometry/Shapes/Headers/Predefined/Quad3D.h"
#include "Platform/File/Headers/FileManagement.h"
#include "Platform/Video/Headers/GFXDevice.h"
#include "Platform/Video/Textures/Headers/Texture.h"
#include "Platform/Video/Shaders/Headers/ShaderProgram.h"

namespace Divide {

GUISplash::GUISplash(ResourceCache& cache,
                     const stringImpl& splashImageName,
                     const vec2<U16>& dimensions) 
    : _dimensions(dimensions)
{
    SamplerDescriptor splashSampler;
    splashSampler.setMinFilter(TextureFilter::NEAREST);
    splashSampler.setAnisotropy(0);
    splashSampler.setWrapMode(TextureWrap::CLAMP);
    // splash shader doesn't do gamma correction since that's a post processing step
    // so fake gamma correction by loading an sRGB image as a linear one.
    splashSampler.toggleSRGBColourSpace(false);

    TextureDescriptor splashDescriptor(TextureType::TEXTURE_2D);
    splashDescriptor.setSampler(splashSampler);

    ResourceDescriptor splashImage("SplashScreen Texture");
    splashImage.setThreadedLoading(false);
    splashImage.setResourceName(splashImageName);
    splashImage.setResourceLocation(Paths::g_assetsLocation + Paths::g_imagesLocation);
    splashImage.setPropertyDescriptor(splashDescriptor);

    _splashImage = CreateResource<Texture>(cache, splashImage);

    ResourceDescriptor splashShader("fbPreview");
    splashShader.setThreadedLoading(false);
    _splashShader = CreateResource<ShaderProgram>(cache, splashShader);
}

GUISplash::~GUISplash()
{
}

void GUISplash::render(GFXDevice& context) {
    GFX::ScopedViewport splashViewport(context, vec4<I32>(0, 0, _dimensions.width, _dimensions.height));
    _splashImage->bind(to_U8(ShaderProgram::TextureUsage::UNIT0));

    PipelineDescriptor pipelineDescriptor;
    pipelineDescriptor._stateHash = context.getDefaultStateBlock(true);
    pipelineDescriptor._shaderProgram = _splashShader;

    GenericDrawCommand triangleCmd;
    triangleCmd.primitiveType(PrimitiveType::TRIANGLES);
    triangleCmd.drawCount(1);

    context.draw(triangleCmd, context.newPipeline(pipelineDescriptor));
}

};