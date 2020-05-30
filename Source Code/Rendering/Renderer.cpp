#include "stdafx.h"

#include "Headers/Renderer.h"

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
    constexpr bool g_useImageLoadStore = false;

    constexpr U16 MAX_HEIGHT = 2160u;
    constexpr U16 MAX_WIDTH = 3840u;

    vec2<U32> GetNumTiles(U8 threadGroupSize) noexcept {
        const U8 TILE_RES = Light::GetThreadGroupSize(threadGroupSize);

        return
        {
            (MAX_WIDTH + (MAX_WIDTH % TILE_RES)) / TILE_RES,
            (MAX_HEIGHT + (MAX_HEIGHT % TILE_RES)) / TILE_RES 
        };
    }

    U32 GetMaxNumTiles(U8 threadGroupSize) noexcept {
        const vec2<U32> tileCount = GetNumTiles(threadGroupSize);
        return tileCount.x * tileCount.y;
    }
};

Renderer::Renderer(PlatformContext& context, ResourceCache* cache)
    : PlatformContextComponent(context),
      _resCache(cache)
{
    Configuration& config = context.config();

    ShaderModuleDescriptor computeDescriptor = {};
    computeDescriptor._moduleType = ShaderType::COMPUTE;
    computeDescriptor._sourceFile = "lightCull.glsl";
    if (g_useImageLoadStore) {
        computeDescriptor._defines.emplace_back("USE_IMAGE_LOAD_STORE", true);
    }

    ShaderProgramDescriptor cullDescritpor = {};
    cullDescritpor._modules.push_back(computeDescriptor);

    ResourceDescriptor cullShaderDesc("lightCull");
    cullShaderDesc.propertyDescriptor(cullDescritpor);
    _lightCullComputeShader = CreateResource<ShaderProgram>(cache, cullShaderDesc);
    PipelineDescriptor pipelineDescriptor = {};
    pipelineDescriptor._shaderProgramHandle = _lightCullComputeShader->getGUID();
    _lightCullPipeline = _context.gfx().newPipeline(pipelineDescriptor);

    U32 totalLights = static_cast<U32>(context.config().rendering.numLightsPerScreenTile);
    totalLights *= GetMaxNumTiles(config.rendering.lightThreadGroupSize);
    I32* bufferData = MemoryManager_NEW I32[totalLights];
    std::memset(bufferData, -1, totalLights * sizeof(I32));

    ShaderBufferDescriptor bufferDescriptor = {};
    bufferDescriptor._usage = ShaderBuffer::Usage::UNBOUND_BUFFER;
    bufferDescriptor._elementCount = totalLights;
    bufferDescriptor._elementSize = sizeof(I32);
    bufferDescriptor._ringBufferLength = 1;
    bufferDescriptor._updateFrequency = BufferUpdateFrequency::ONCE;
    bufferDescriptor._updateUsage = BufferUpdateUsage::GPU_R_GPU_W;
    bufferDescriptor._name = "PER_TILE_LIGHT_INDEX";
    bufferDescriptor._initialData = bufferData;
    _perTileLightIndexBuffer = _context.gfx().newSB(bufferDescriptor);
    _perTileLightIndexBuffer->bind(ShaderBufferLocation::LIGHT_INDICES);
    MemoryManager::DELETE_ARRAY(bufferData);

    _postFX = eastl::make_unique<PostFX>(context, cache);
    if (config.rendering.postFX.PostAAQualityLevel > 0) {
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
    if (config.rendering.postFX.enablePerObjectMotionBlur) {
        _postFX->pushFilter(FilterType::FILTER_MOTION_BLUR);
    }
    if (config.rendering.postFX.enableBloom) {
        _postFX->pushFilter(FilterType::FILTER_BLOOM);
    }
    if (false) {
        _postFX->pushFilter(FilterType::FILTER_LUT_CORECTION);
    }

    WAIT_FOR_CONDITION(_lightCullPipeline != nullptr);
}

Renderer::~Renderer()
{
    // Destroy our post processing system
    Console::printfn(Locale::get(_ID("STOP_POST_FX")));
}

void Renderer::preRender(RenderStagePass stagePass,
                         const Texture_ptr& hizColourTexture,
                         LightPool& lightPool,
                         const Camera* camera,
                         GFX::CommandBuffer& bufferInOut) {

    if (stagePass._stage == RenderStage::SHADOW) {
        return;
    }

    lightPool.uploadLightData(stagePass._stage, bufferInOut);
    if (!hizColourTexture) {
        return;
    }

    const vec2<U16> renderTargetRes(hizColourTexture->width(), hizColourTexture->height());
    const Rect<I32> viewport(0u, 0u, renderTargetRes.x, renderTargetRes.y);

    GFX::EnqueueCommand(bufferInOut, GFX::BeginDebugScopeCommand{ "Renderer PrePass" });

    GFX::BindPipelineCommand bindPipelineCmd = {};
    bindPipelineCmd._pipeline = _lightCullPipeline;
    GFX::EnqueueCommand(bufferInOut, bindPipelineCmd);

    GFX::BindDescriptorSetsCommand bindDescriptorSetsCmd = {};
    if (g_useImageLoadStore) {
        Image depthImage = {};
        depthImage._texture = hizColourTexture.get();
        depthImage._flag = Image::Flag::READ;
        depthImage._binding = to_U8(TextureUsage::DEPTH);
        depthImage._layer = 0u;
        depthImage._level = 0u;

        bindDescriptorSetsCmd._set._images.push_back(depthImage);
    } else {
        bindDescriptorSetsCmd._set._textureData.setTexture(hizColourTexture->data(), TextureUsage::DEPTH);
    }

    GFX::EnqueueCommand(bufferInOut, bindDescriptorSetsCmd);

    GFX::SendPushConstantsCommand preRenderPushConstantsCmd;
    preRenderPushConstantsCmd._constants.countHint(4);
    preRenderPushConstantsCmd._constants.set(_ID("viewMatrix"), GFX::PushConstantType::MAT4, camera->getViewMatrix());
    preRenderPushConstantsCmd._constants.set(_ID("projectionMatrix"), GFX::PushConstantType::MAT4, camera->getProjectionMatrix());
    preRenderPushConstantsCmd._constants.set(_ID("viewProjectionMatrix"), GFX::PushConstantType::MAT4, mat4<F32>::Multiply(camera->getViewMatrix(), camera->getProjectionMatrix()));
    preRenderPushConstantsCmd._constants.set(_ID("viewportDimensions"), GFX::PushConstantType::VEC2, vec2<F32>(renderTargetRes));
    GFX::EnqueueCommand(bufferInOut, preRenderPushConstantsCmd);

    const U8  tileRes = Light::GetThreadGroupSize(_context.config().rendering.lightThreadGroupSize);
    const U32 workGroupsX = (renderTargetRes.x + (renderTargetRes.x % tileRes)) / tileRes;
    const U32 workGroupsY = (renderTargetRes.y + (renderTargetRes.y % tileRes)) / tileRes;

    GFX::DispatchComputeCommand computeCmd = {};
    computeCmd._computeGroupSize.set(workGroupsX, workGroupsY, 1);
    GFX::EnqueueCommand(bufferInOut, computeCmd);

    GFX::MemoryBarrierCommand memCmd = {};
    memCmd._barrierMask = to_base(MemoryBarrierType::BUFFER_UPDATE);
    GFX::EnqueueCommand(bufferInOut, memCmd);

    GFX::EndDebugScopeCommand endDebugScopeCmd;
    GFX::EnqueueCommand(bufferInOut, endDebugScopeCmd);
}

void Renderer::idle() {
    OPTICK_EVENT();

    _postFX->idle(_context.config());
}

void Renderer::updateResolution(U16 newWidth, U16 newHeight) {
    _postFX->updateResolution(newWidth, newHeight);
}
};