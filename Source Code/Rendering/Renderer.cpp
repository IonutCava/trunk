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
      _numLightsPerTile(1u),
      _debugView(false)
{
    ResourceDescriptor cullShaderDesc("lightCull");
    cullShaderDesc.setThreadedLoading(false);
    _lightCullComputeShader = CreateResource<ShaderProgram>(cache, cullShaderDesc);

    RenderTarget& screenRT = _context.gfx().renderTargetPool().renderTarget(RenderTargetID(RenderTargetUsage::SCREEN));

    const U32 numTiles = getNumTilesX(screenRT.getWidth()) * getNumTilesY(screenRT.getHeight());
    _numLightsPerTile = getMaxNumLightsPerTile(screenRT.getHeight());
    assert(_numLightsPerTile > 1);


    ShaderBufferDescriptor bufferDescriptor;
    bufferDescriptor._elementCount = _numLightsPerTile * numTiles;
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

    _numLightsPerTile = getMaxNumLightsPerTile(target.getHeight());

    GFX::BindDescriptorSetsCommand bindDescriptorSetsCmd = {};
    TextureData data = target.getAttachment(RTAttachmentType::Depth, 0).texture()->getData();
    bindDescriptorSetsCmd._set._textureData.addTexture(data, to_U8(ShaderProgram::TextureUsage::DEPTH));
    GFX::EnqueueCommand(bufferInOut, bindDescriptorSetsCmd);

    _context.gfx().preRender(stagePass, _numLightsPerTile, bufferInOut);

    if (stagePass._stage == RenderStage::SHADOW) {
        return;
    }

    GFX::BindPipelineCommand bindPipelineCmd = {};
    PipelineDescriptor pipelineDescriptor = {};
    pipelineDescriptor._shaderProgramHandle = _lightCullComputeShader->getID();
    bindPipelineCmd._pipeline = _context.gfx().newPipeline(pipelineDescriptor);
    GFX::EnqueueCommand(bufferInOut, bindPipelineCmd);

    GFX::SendPushConstantsCommand sendPushConstantsCmd = {};
    sendPushConstantsCmd._constants.set("numDirLights", GFX::PushConstantType::UINT, lightPool.getActiveLightCount(stagePass._stage, LightType::DIRECTIONAL));
    sendPushConstantsCmd._constants.set("numPointLights", GFX::PushConstantType::UINT, stagePass._stage == RenderStage::DISPLAY ? lightPool.getActiveLightCount(stagePass._stage, LightType::POINT) : 0);
    sendPushConstantsCmd._constants.set("numSpotLights", GFX::PushConstantType::UINT, stagePass._stage == RenderStage::DISPLAY ? lightPool.getActiveLightCount(stagePass._stage, LightType::SPOT) : 0);
    GFX::EnqueueCommand(bufferInOut, sendPushConstantsCmd);

    GFX::DispatchComputeCommand computeCmd = {};
    computeCmd._computeGroupSize.set(getNumTilesX(target.getWidth()), getNumTilesY(target.getHeight()), 1);
    assert(computeCmd._computeGroupSize.lengthSquared() > 0);
    GFX::EnqueueCommand(bufferInOut, computeCmd);

    GFX::MemoryBarrierCommand memCmd = {};
    memCmd._barrierMask = to_base(MemoryBarrierType::SHADER_BUFFER);
    GFX::EnqueueCommand(bufferInOut, memCmd);
}

U16 Renderer::getMaxNumLightsPerTile(U16 height) const {
    const U16 adjustmentMultipier = 32;

    // I haven't tested at greater than 1080p, so cap it
    height = std::min(height, to_U16(1080));
    // adjust max lights per tile down as height increases
    return (Config::Lighting::FORWARD_PLUS_MAX_LIGHTS_PER_TILE - (adjustmentMultipier * (height / 120)));
}

U16 Renderer::getNumTilesX(U16 width) const {
    return to_U16((width + Config::Lighting::FORWARD_PLUS_TILE_RES - 1) /
           to_F32(Config::Lighting::FORWARD_PLUS_TILE_RES));
}

U16 Renderer::getNumTilesY(U16 height) const {
    return to_U16((height + Config::Lighting::FORWARD_PLUS_TILE_RES - 1) /
           to_F32(Config::Lighting::FORWARD_PLUS_TILE_RES));
}
};