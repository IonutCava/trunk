#include "stdafx.h"

#include "Headers/Renderer.h"

#include "Core/Headers/Console.h"
#include "Core/Headers/Configuration.h"
#include "Core/Headers/PlatformContext.h"
#include "Core/Resources/Headers/ResourceCache.h"

#include "Rendering/PostFX/Headers/PostFX.h"
#include "Rendering/Camera/Headers/Camera.h"
#include "Rendering/Lighting/Headers/LightPool.h"

#include "Platform/Video/Textures/Headers/Texture.h"
#include "Platform/Video/Shaders/Headers/ShaderProgram.h"
#include "Platform/Video/Buffers/ShaderBuffer/Headers/ShaderBuffer.h"

namespace Divide {

namespace {
    constexpr unsigned short MAX_HEIGHT = 2160u;
    constexpr unsigned short MAX_WIDTH = 3840u;

    constexpr unsigned int NUM_TILES_X = (MAX_WIDTH + (MAX_WIDTH % Config::Lighting::ForwardPlus::TILE_RES)) / Config::Lighting::ForwardPlus::TILE_RES;
    constexpr unsigned int NUM_TILES_Y = (MAX_HEIGHT + (MAX_HEIGHT % Config::Lighting::ForwardPlus::TILE_RES)) / Config::Lighting::ForwardPlus::TILE_RES;

    constexpr U32 g_maxNumberOfTiles = NUM_TILES_X * NUM_TILES_Y;
};

Renderer::Renderer(PlatformContext& context, ResourceCache& cache)
    : PlatformContextComponent(context),
      _resCache(cache)
{
    Configuration& config = context.config();

    ShaderModuleDescriptor computeDescriptor = {};
    computeDescriptor._moduleType = ShaderType::COMPUTE;
    computeDescriptor._sourceFile = "lightCull.glsl";

    ShaderProgramDescriptor cullDescritpor = {};
    cullDescritpor._modules.push_back(computeDescriptor);

    ResourceDescriptor cullShaderDesc("lightCull");
    cullShaderDesc.threaded(false);
    cullShaderDesc.propertyDescriptor(cullDescritpor);

    _lightCullComputeShader = CreateResource<ShaderProgram>(cache, cullShaderDesc);

    I32 numLightsPerTile = context.config().rendering.numLightsPerScreenTile;
    if (numLightsPerTile < 0) {
        numLightsPerTile = to_I32(Config::Lighting::ForwardPlus::MAX_LIGHTS_PER_TILE);
    } else {
        CLAMP(numLightsPerTile, 0, to_I32(Config::Lighting::ForwardPlus::MAX_LIGHTS_PER_TILE));
    }
    vectorEASTL<I32> initData(numLightsPerTile * g_maxNumberOfTiles, -1);

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

    _postFX = std::make_unique<PostFX>(context, cache);
    if (config.rendering.postFX.postAASamples > 0) {
        _postFX->pushFilter(FilterType::FILTER_SS_ANTIALIASING);
    }
    if (false) {
        _postFX->pushFilter(FilterType::FILTER_SS_REFLECTIONS);
    }
    if (config.rendering.postFX.enableSSAO) {
        _postFX->pushFilter(FilterType::FILTER_SS_AMBIENT_OCCLUSION);
    }
    if (config.rendering.postFX.enableDepthOfField) {
        _postFX->pushFilter(FilterType::FILTER_DEPTH_OF_FIELD);
    }
    if (false) {
        _postFX->pushFilter(FilterType::FILTER_MOTION_BLUR);
    }
    if (config.rendering.postFX.enableBloom) {
        _postFX->pushFilter(FilterType::FILTER_BLOOM);
    }
    if (false) {
        _postFX->pushFilter(FilterType::FILTER_LUT_CORECTION);
    }
}

Renderer::~Renderer()
{
    // Destroy our post processing system
    Console::printfn(Locale::get(_ID("STOP_POST_FX")));
}

void Renderer::preRender(RenderStagePass stagePass,
                         const Texture_ptr& hizColourTexture,
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
    if (!hizColourTexture) {
        return;
    }

    const vec2<U16> renderTargetRes(hizColourTexture->width(), hizColourTexture->height());
    const Rect<I32> viewport(0u, 0u, renderTargetRes.x, renderTargetRes.y);

    GFX::BeginDebugScopeCommand beginDebugScopeCmd;
    beginDebugScopeCmd._scopeID = to_I32(stagePass.index());
    beginDebugScopeCmd._scopeName = "Renderer PrePass";
    GFX::EnqueueCommand(bufferInOut, beginDebugScopeCmd);

    GFX::BindPipelineCommand bindPipelineCmd = {};
    bindPipelineCmd._pipeline = pipeline;
    GFX::EnqueueCommand(bufferInOut, bindPipelineCmd);

    Image depthImage = {};
    depthImage._texture = hizColourTexture.get();
    depthImage._flag = Image::Flag::READ;
    depthImage._binding = to_U8(ShaderProgram::TextureUsage::DEPTH);
    depthImage._layer = 0u;
    depthImage._level = 0u;

    GFX::BindDescriptorSetsCommand bindDescriptorSetsCmd = {};
    bindDescriptorSetsCmd._set._images.push_back(depthImage);
    GFX::EnqueueCommand(bufferInOut, bindDescriptorSetsCmd);

    GFX::SendPushConstantsCommand preRenderPushConstantsCmd;
    preRenderPushConstantsCmd._constants.countHint(5);
    preRenderPushConstantsCmd._constants.set("viewMatrix", GFX::PushConstantType::MAT4, camera.getViewMatrix());
    preRenderPushConstantsCmd._constants.set("viewportDimensions", GFX::PushConstantType::VEC2, vec2<F32>(renderTargetRes));
    preRenderPushConstantsCmd._constants.set("projectionMatrix", GFX::PushConstantType::MAT4, camera.getProjectionMatrix());
    preRenderPushConstantsCmd._constants.set("invProjectionMatrix", GFX::PushConstantType::MAT4, camera.getProjectionMatrix().getInverse());
    preRenderPushConstantsCmd._constants.set("viewProjectionMatrix", GFX::PushConstantType::MAT4, mat4<F32>::Multiply(camera.getViewMatrix(), camera.getProjectionMatrix()));
    GFX::EnqueueCommand(bufferInOut, preRenderPushConstantsCmd);

    constexpr U32 tileRes = Config::Lighting::ForwardPlus::TILE_RES;
    const U32 workGroupsX = (renderTargetRes.x + (renderTargetRes.x % tileRes)) / tileRes;
    const U32 workGroupsY = (renderTargetRes.y + (renderTargetRes.y % tileRes)) / tileRes;

    GFX::DispatchComputeCommand computeCmd = {};
    computeCmd._computeGroupSize.set(workGroupsX, workGroupsY, 1);
    GFX::EnqueueCommand(bufferInOut, computeCmd);

    GFX::MemoryBarrierCommand memCmd = {};
    memCmd._barrierMask = to_base(MemoryBarrierType::SHADER_BUFFER);
    GFX::EnqueueCommand(bufferInOut, memCmd);

    GFX::EndDebugScopeCommand endDebugScopeCmd;
    GFX::EnqueueCommand(bufferInOut, endDebugScopeCmd);
}

void Renderer::idle() {
    _postFX->idle(_context.config());
}

void Renderer::updateResolution(U16 newWidth, U16 newHeight) {
    _postFX->updateResolution(newWidth, newHeight);
}
};