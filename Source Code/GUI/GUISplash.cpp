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
    PipelineDescriptor pipelineDescriptor;
    pipelineDescriptor._stateHash = context.get2DStateBlock();
    pipelineDescriptor._shaderProgram = _splashShader;

    ScopedCommandBuffer sBuffer = context.allocateScopedCommandBuffer();
    GFX::CommandBuffer& buffer = sBuffer();

    GFX::BindPipelineCommand pipelineCmd;
    pipelineCmd._pipeline = context.newPipeline(pipelineDescriptor);
    GFX::BindPipeline(buffer, pipelineCmd);


    GFX::SetViewportCommand viewportCommand;
    viewportCommand._viewport.set(0, 0, _dimensions.width, _dimensions.height);
    GFX::SetViewPort(buffer, viewportCommand);

    GFX::BindDescriptorSetsCommand descriptorSetCmd;
    descriptorSetCmd._set._textureData.addTexture(_splashImage->getData(),
                                                  to_U8(ShaderProgram::TextureUsage::UNIT0));
    GFX::BindDescriptorSets(buffer, descriptorSetCmd);

    GenericDrawCommand triangleCmd;
    triangleCmd.primitiveType(PrimitiveType::TRIANGLES);
    triangleCmd.drawCount(1);

    GFX::DrawCommand drawCmd;
    drawCmd._drawCommands.push_back(triangleCmd);
    GFX::AddDrawCommands(buffer, drawCmd);

    context.flushCommandBuffer(buffer);
}

};