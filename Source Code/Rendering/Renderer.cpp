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

Renderer::Renderer(PlatformContext& context, ResourceCache& cache)
    : PlatformContextComponent(context),
      _resCache(cache),
      _debugView(false)
{
    ResourceDescriptor cullShaderDesc("lightCull");
    cullShaderDesc.setThreadedLoading(false);
    _lightCullComputeShader = CreateResource<ShaderProgram>(cache, cullShaderDesc);

    ShaderBufferDescriptor bufferDescriptor;
    bufferDescriptor._elementCount = Config::Lighting::ForwardPlus::NUM_LIGTHS_PER_TILE * Config::Lighting::ForwardPlus::NUM_TILES_X * Config::Lighting::ForwardPlus::NUM_TILES_Y;
    bufferDescriptor._elementSize = sizeof(U32);
    bufferDescriptor._ringBufferLength = 1;
    bufferDescriptor._flags = to_U32(ShaderBuffer::Flags::UNBOUND_STORAGE);
    bufferDescriptor._updateFrequency = BufferUpdateFrequency::ONCE;
    bufferDescriptor._name = "PER_TILE_LIGHT_INDEX";
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

    lightPool.uploadLightData(stagePass._stage, ShaderBufferLocation::LIGHT_NORMAL, ShaderBufferLocation::LIGHT_SHADOW, bufferInOut);


    GFX::BindDescriptorSetsCommand bindDescriptorSetsCmd = {};
    TextureData data = target.getAttachment(RTAttachmentType::Depth, 0).texture()->getData();
    bindDescriptorSetsCmd._set._textureData.addTexture(data, to_U8(ShaderProgram::TextureUsage::DEPTH));
    GFX::EnqueueCommand(bufferInOut, bindDescriptorSetsCmd);

    _context.gfx().preRender(stagePass, bufferInOut);

    if (stagePass._stage == RenderStage::SHADOW) {
        return;
    }

    GFX::BindPipelineCommand bindPipelineCmd = {};
    PipelineDescriptor pipelineDescriptor = {};
    pipelineDescriptor._shaderProgramHandle = _lightCullComputeShader->getID();
    bindPipelineCmd._pipeline = _context.gfx().newPipeline(pipelineDescriptor);
    GFX::EnqueueCommand(bufferInOut, bindPipelineCmd);

    GFX::DispatchComputeCommand computeCmd = {};
    computeCmd._computeGroupSize.set(Config::Lighting::ForwardPlus::NUM_TILES_X, Config::Lighting::ForwardPlus::NUM_TILES_Y, 1);
    assert(computeCmd._computeGroupSize.lengthSquared() > 0);
    GFX::EnqueueCommand(bufferInOut, computeCmd);

    GFX::MemoryBarrierCommand memCmd = {};
    memCmd._barrierMask = to_base(MemoryBarrierType::SHADER_BUFFER);
    GFX::EnqueueCommand(bufferInOut, memCmd);
}

};