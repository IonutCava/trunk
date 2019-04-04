#include "stdafx.h"

#include "Headers/Renderer.h"

#include "Core/Headers/Console.h"
#include "Core/Headers/PlatformContext.h"
#include "Core/Resources/Headers/ResourceCache.h"
#include "Rendering/Lighting/Headers/LightPool.h"
#include "Platform/Video/Textures/Headers/Texture.h"
#include "Platform/Video/Shaders/Headers/ShaderProgram.h"
#include "Platform/Video/Buffers/ShaderBuffer/Headers/ShaderBuffer.h"

namespace Divide {

namespace {
    constexpr U32 g_numberOfTiles = Config::Lighting::ForwardPlus::NUM_TILES_X * Config::Lighting::ForwardPlus::NUM_TILES_Y;
};

Renderer::Renderer(PlatformContext& context, ResourceCache& cache)
    : PlatformContextComponent(context),
      _resCache(cache),
      _debugView(false)
{
    ResourceDescriptor cullShaderDesc("lightCull");
    cullShaderDesc.setThreadedLoading(false);
    _lightCullComputeShader = CreateResource<ShaderProgram>(cache, cullShaderDesc);

    vector<I32> initData(Config::Lighting::ForwardPlus::MAX_LIGHTS_PER_TILE * g_numberOfTiles, -1);

    ShaderBufferDescriptor bufferDescriptor;
    bufferDescriptor._usage = ShaderBuffer::Usage::UNBOUND_BUFFER;
    bufferDescriptor._elementCount = Config::Lighting::ForwardPlus::MAX_LIGHTS_PER_TILE * g_numberOfTiles;
    bufferDescriptor._elementSize = sizeof(I32);
    bufferDescriptor._ringBufferLength = 1;
    bufferDescriptor._updateFrequency = BufferUpdateFrequency::ONCE;
    bufferDescriptor._name = "PER_TILE_LIGHT_INDEX";
    bufferDescriptor._initialData = initData.data();
    _perTileLightIndexBuffer = _context.gfx().newSB(bufferDescriptor);
    _perTileLightIndexBuffer->bind(ShaderBufferLocation::LIGHT_INDICES);
}

Renderer::~Renderer()
{
}

void Renderer::preRender(RenderStagePass stagePass,
                         RenderTarget& target,
                         LightPool& lightPool,
                         GFX::CommandBuffer& bufferInOut) {

    const Texture_ptr& depthTexture = target.getAttachment(RTAttachmentType::Depth, 0).texture();

    Image img = {};
    img._texture = depthTexture.get();
    img._binding = to_U8(ShaderProgram::TextureUsage::DEPTH);
    img._layer = 0;
    img._level = 0;
    img._flag = Image::Flag::READ;

    GFX::BindDescriptorSetsCommand bindDescriptorSetsCmd = {};
    //bindDescriptorSetsCmd._set._images.push_back(img);
    bindDescriptorSetsCmd._set._textureData.setTexture(depthTexture->getData(), to_U8(ShaderProgram::TextureUsage::DEPTH));
    GFX::EnqueueCommand(bufferInOut, bindDescriptorSetsCmd);

    if (stagePass._stage == RenderStage::SHADOW) {
        return;
    }

    lightPool.uploadLightData(stagePass._stage, bufferInOut);
    if (stagePass._stage != RenderStage::DISPLAY) {
        return;
    }

    GFX::SetViewportCommand viewportCommand;
    viewportCommand._viewport.set(Rect<I32>(0, 0, depthTexture->getWidth(), depthTexture->getHeight()));
    GFX::EnqueueCommand(bufferInOut, viewportCommand);

    GFX::BindPipelineCommand bindPipelineCmd = {};
    PipelineDescriptor pipelineDescriptor = {};
    pipelineDescriptor._shaderProgramHandle = _lightCullComputeShader->getID();
    bindPipelineCmd._pipeline = _context.gfx().newPipeline(pipelineDescriptor);
    GFX::EnqueueCommand(bufferInOut, bindPipelineCmd);

    GFX::DispatchComputeCommand computeCmd = {};
    computeCmd._computeGroupSize.set(
        Config::Lighting::ForwardPlus::NUM_TILES_X,
        Config::Lighting::ForwardPlus::NUM_TILES_Y,
        1);

    assert(computeCmd._computeGroupSize.lengthSquared() > 0);
    GFX::EnqueueCommand(bufferInOut, computeCmd);

    GFX::MemoryBarrierCommand memCmd = {};
    memCmd._barrierMask = to_base(MemoryBarrierType::SHADER_BUFFER);
    GFX::EnqueueCommand(bufferInOut, memCmd);
}

};