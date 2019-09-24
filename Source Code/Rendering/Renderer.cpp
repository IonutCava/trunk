#include "stdafx.h"

#include "Headers/Renderer.h"

#include "Core/Headers/Console.h"
#include "Core/Headers/Configuration.h"
#include "Core/Headers/PlatformContext.h"
#include "Core/Resources/Headers/ResourceCache.h"
#include "Rendering/Camera/Headers/Camera.h"
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
      _resCache(cache)
{
    ShaderModuleDescriptor computeDescriptor = {};
    computeDescriptor._moduleType = ShaderType::COMPUTE;
    computeDescriptor._sourceFile = "lightCull.glsl";

    ShaderProgramDescriptor cullDescritpor = {};
    cullDescritpor._modules.push_back(computeDescriptor);

    ResourceDescriptor cullShaderDesc("lightCull");
    cullShaderDesc.setThreadedLoading(false);
    cullShaderDesc.setPropertyDescriptor(cullDescritpor);

    _lightCullComputeShader = CreateResource<ShaderProgram>(cache, cullShaderDesc);

    I32 numLightsPerTile = context.config().rendering.numLightsPerScreenTile;
    if (numLightsPerTile < 0) {
        numLightsPerTile = to_I32(Config::Lighting::ForwardPlus::MAX_LIGHTS_PER_TILE);
    } else {
        CLAMP(numLightsPerTile, 0, to_I32(Config::Lighting::ForwardPlus::MAX_LIGHTS_PER_TILE));
    }
    vectorEASTL<I32> initData(numLightsPerTile * g_numberOfTiles, -1);

    ShaderBufferDescriptor bufferDescriptor;
    bufferDescriptor._usage = ShaderBuffer::Usage::UNBOUND_BUFFER;
    bufferDescriptor._elementCount = to_U32(initData.size());
    bufferDescriptor._elementSize = sizeof(I32);
    bufferDescriptor._ringBufferLength = 1;
    bufferDescriptor._updateFrequency = BufferUpdateFrequency::ONCE;
    bufferDescriptor._updateUsage = BufferUpdateUsage::GPU_R_GPU_W;
    bufferDescriptor._name = "PER_TILE_LIGHT_INDEX";
    bufferDescriptor._initialData = initData.data();
    _perTileLightIndexBuffer = _context.gfx().newSB(bufferDescriptor);
    _perTileLightIndexBuffer->bind(ShaderBufferLocation::LIGHT_INDICES);
}

Renderer::~Renderer()
{
}

void Renderer::preRender(RenderStagePass stagePass,
                         RenderTargetID target,
                         LightPool& lightPool,
                         const Camera& camera,
                         GFX::CommandBuffer& bufferInOut) {

    static Pipeline* pipeline = nullptr;
    if (pipeline == nullptr) {
        PipelineDescriptor pipelineDescriptor = {};
        pipelineDescriptor._shaderProgramHandle = _lightCullComputeShader->getGUID();
        pipeline = _context.gfx().newPipeline(pipelineDescriptor);
    }

    if (stagePass._stage == RenderStage::SHADOW) {
        return;
    }

    lightPool.uploadLightData(stagePass._stage, bufferInOut);

    const RenderTarget& rt = _context.gfx().renderTargetPool().renderTarget(target);
    GFX::SetViewportCommand viewportCommand = {};
    viewportCommand._viewport.set(Rect<I32>(0, 0, rt.getWidth(), rt.getHeight()));
    GFX::EnqueueCommand(bufferInOut, viewportCommand);

    GFX::BindPipelineCommand bindPipelineCmd = {};
    bindPipelineCmd._pipeline = pipeline;
    GFX::EnqueueCommand(bufferInOut, bindPipelineCmd);

    GFX::SendPushConstantsCommand preRenderPushConstantsCmd;
    preRenderPushConstantsCmd._constants.countHint(3);
    preRenderPushConstantsCmd._constants.set("viewMatrix", GFX::PushConstantType::MAT4, camera.getViewMatrix());
    preRenderPushConstantsCmd._constants.set("viewportDimensions", GFX::PushConstantType::VEC2, vec2<F32>(rt.getWidth(), rt.getHeight()));
    preRenderPushConstantsCmd._constants.set("projectionMatrix", GFX::PushConstantType::MAT4, camera.getProjectionMatrix());
    GFX::EnqueueCommand(bufferInOut, preRenderPushConstantsCmd);

    GFX::DispatchComputeCommand computeCmd = {};
    computeCmd._computeGroupSize.set(
        Config::Lighting::ForwardPlus::NUM_TILES_X,
        Config::Lighting::ForwardPlus::NUM_TILES_Y,
        1);
    GFX::EnqueueCommand(bufferInOut, computeCmd);

    GFX::MemoryBarrierCommand memCmd = {};
    memCmd._barrierMask = to_base(MemoryBarrierType::SHADER_BUFFER);
    GFX::EnqueueCommand(bufferInOut, memCmd);
}

};