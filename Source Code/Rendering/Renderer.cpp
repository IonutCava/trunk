#include "stdafx.h"

#include "Headers/Renderer.h"

#include "Core/Headers/Configuration.h"
#include "Core/Headers/PlatformContext.h"
#include "Core/Resources/Headers/ResourceCache.h"

#include "Rendering/Camera/Headers/Camera.h"
#include "Rendering/Lighting/Headers/LightPool.h"
#include "Rendering/PostFX/Headers/PostFX.h"

#include "Platform/Video/Buffers/ShaderBuffer/Headers/ShaderBuffer.h"
#include "Platform/Video/Shaders/Headers/ShaderProgram.h"

namespace Divide {

Renderer::Renderer(PlatformContext& context, ResourceCache* cache)
    : PlatformContextComponent(context)
{
    const Configuration& config = context.config();
    const U32 numClusters = to_U32(config.rendering.lightClusteredSizes.x) *
                            to_U32(config.rendering.lightClusteredSizes.y) *
                            to_U32(config.rendering.lightClusteredSizes.z);

    ShaderModuleDescriptor computeDescriptor = {};
    computeDescriptor._moduleType = ShaderType::COMPUTE;

    {
        computeDescriptor._sourceFile = "lightCull.glsl";
        ShaderProgramDescriptor cullDescritpor = {};
        cullDescritpor._modules.push_back(computeDescriptor);

        ResourceDescriptor cullShaderDesc("lightCull");
        cullShaderDesc.propertyDescriptor(cullDescritpor);
        _lightCullComputeShader = CreateResource<ShaderProgram>(cache, cullShaderDesc);
        PipelineDescriptor pipelineDescriptor = {};
        pipelineDescriptor._shaderProgramHandle = _lightCullComputeShader->getGUID();
        _lightCullPipeline = _context.gfx().newPipeline(pipelineDescriptor);
    }
    {
        computeDescriptor._sourceFile = "lightBuildClusteredAABBs.glsl";
        ShaderProgramDescriptor buildDescritpor = {};
        buildDescritpor._modules.push_back(computeDescriptor);
        ResourceDescriptor buildShaderDesc("lightBuildClusteredAABBs");
        buildShaderDesc.propertyDescriptor(buildDescritpor);
        _lightBuildClusteredAABBsComputeShader = CreateResource<ShaderProgram>(cache, buildShaderDesc);

        PipelineDescriptor pipelineDescriptor = {};
        pipelineDescriptor._shaderProgramHandle = _lightBuildClusteredAABBsComputeShader->getGUID();
        _lightBuildClusteredAABBsPipeline = _context.gfx().newPipeline(pipelineDescriptor);
    }


    ShaderBufferDescriptor bufferDescriptor = {};
    bufferDescriptor._usage = ShaderBuffer::Usage::UNBOUND_BUFFER;
    bufferDescriptor._ringBufferLength = 1;
    { //Light Index Buffer
        const U32 totalLights = numClusters * to_U32(config.rendering.numLightsPerCluster);
        bufferDescriptor._name = "LIGHT_INDEX_SSBO";
        bufferDescriptor._bufferParams._elementCount = totalLights;
        bufferDescriptor._bufferParams._elementSize = sizeof(U32);
        bufferDescriptor._bufferParams._updateFrequency = BufferUpdateFrequency::ONCE;
        bufferDescriptor._bufferParams._updateUsage = BufferUpdateUsage::GPU_R_GPU_W;
        bufferDescriptor._bufferParams._initialData = { nullptr, 0 };
        _lightIndexBuffer = _context.gfx().newSB(bufferDescriptor);
        _lightIndexBuffer->bind(ShaderBufferLocation::LIGHT_INDICES);
    }
    { // Light Grid Buffer
        bufferDescriptor._name = "LIGHT_GRID_SSBO";
        bufferDescriptor._bufferParams._elementCount = numClusters;
        bufferDescriptor._bufferParams._elementSize = 3 * sizeof(U32);
        bufferDescriptor._bufferParams._updateFrequency = BufferUpdateFrequency::ONCE;
        bufferDescriptor._bufferParams._updateUsage = BufferUpdateUsage::GPU_R_GPU_W;
        bufferDescriptor._bufferParams._initialData = { nullptr, 0 };
        _lightGridBuffer = _context.gfx().newSB(bufferDescriptor);
        _lightGridBuffer->bind(ShaderBufferLocation::LIGHT_GRID);
    }
    { // Global Index Count
        bufferDescriptor._name = "GLOBAL_INDEX_COUNT_SSBO";
        bufferDescriptor._bufferParams._elementSize = sizeof(U32);
        bufferDescriptor._bufferParams._updateFrequency = BufferUpdateFrequency::ONCE;
        bufferDescriptor._bufferParams._updateUsage = BufferUpdateUsage::GPU_R_GPU_W;
        bufferDescriptor._bufferParams._initialData = { nullptr, 0 };
        _globalIndexCountBuffer = _context.gfx().newSB(bufferDescriptor);
        _globalIndexCountBuffer->bind(ShaderBufferLocation::LIGHT_INDEX_COUNT);
    }
    { // Cluster AABBs
        bufferDescriptor._bufferParams._elementCount = numClusters;
        bufferDescriptor._bufferParams._elementSize = 2 * (4 * sizeof(F32));
        bufferDescriptor._bufferParams._updateFrequency = BufferUpdateFrequency::ONCE;
        bufferDescriptor._bufferParams._updateUsage = BufferUpdateUsage::GPU_R_GPU_W;
        bufferDescriptor._bufferParams._initialData = { nullptr, 0 };
        _lightClusterAABBsBuffer = _context.gfx().newSB(bufferDescriptor);
        _lightClusterAABBsBuffer->bind(ShaderBufferLocation::LIGHT_CLUSTER_AABBS);
    }

    _postFX = eastl::make_unique<PostFX>(context, cache);
    if (config.rendering.postFX.PostAAQualityLevel > 0) {
        _postFX->pushFilter(FilterType::FILTER_SS_ANTIALIASING);
    }
    if (config.rendering.postFX.enableScreenSpaceReflections) {
        _postFX->pushFilter(FilterType::FILTER_SS_REFLECTIONS);
    }
    if (config.rendering.postFX.ssao.enable) {
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
    if_constexpr(false) {
        _postFX->pushFilter(FilterType::FILTER_LUT_CORECTION);
    }

    WAIT_FOR_CONDITION(_lightBuildClusteredAABBsPipeline != nullptr);
    WAIT_FOR_CONDITION(_lightCullPipeline != nullptr);
}

Renderer::~Renderer()
{
    // Destroy our post processing system
    Console::printfn(Locale::get(_ID("STOP_POST_FX")));
}

void Renderer::preRender(RenderStagePass stagePass,
                         const Texture_ptr& hizColourTexture,
                         const size_t samplerHash,
                         LightPool& lightPool,
                         const Camera* camera,
                         GFX::CommandBuffer& bufferInOut)
{
    ACKNOWLEDGE_UNUSED(samplerHash);

    if (stagePass._stage == RenderStage::SHADOW) {
        return;
    }

    lightPool.uploadLightData(stagePass._stage, bufferInOut);
    if (!hizColourTexture) {
        return;
    }

    const Configuration& config = _context.config();
    const U32 gridSizeZ = to_U32(config.rendering.lightClusteredSizes.z);
    const U32 zThreads = gridSizeZ / Config::Lighting::ClusteredForward::CLUSTER_Z_THREADS;

    GFX::BindPipelineCommand bindPipelineCmd = {};
    GFX::DispatchComputeCommand computeCmd = {};
    GFX::MemoryBarrierCommand memCmd = {};

    const mat4<F32>& projectionMatrix = camera->projectionMatrix();
    if (_previousProjMatrix != projectionMatrix) {
        _previousProjMatrix = projectionMatrix;

        EnqueueCommand(bufferInOut, GFX::BeginDebugScopeCommand{ "Renderer Rebuild Light Grid" });

        bindPipelineCmd._pipeline = _lightBuildClusteredAABBsPipeline;
        EnqueueCommand(bufferInOut, bindPipelineCmd);

        computeCmd._computeGroupSize.set(1, 1, zThreads);
        EnqueueCommand(bufferInOut, computeCmd);

        memCmd._barrierMask = to_base(MemoryBarrierType::SHADER_STORAGE);
        EnqueueCommand(bufferInOut, memCmd);

        EnqueueCommand(bufferInOut, GFX::EndDebugScopeCommand{});
    }

    EnqueueCommand(bufferInOut, GFX::BeginDebugScopeCommand{ "Renderer Cull Lights" });

    bindPipelineCmd._pipeline = _lightCullPipeline;
    EnqueueCommand(bufferInOut, bindPipelineCmd);

    computeCmd._computeGroupSize.set(1, 1, zThreads);
    EnqueueCommand(bufferInOut, computeCmd);

    memCmd._barrierMask = to_base(MemoryBarrierType::BUFFER_UPDATE);
    EnqueueCommand(bufferInOut, memCmd);

    EnqueueCommand(bufferInOut, GFX::EndDebugScopeCommand{});
}

void Renderer::idle() const {
    OPTICK_EVENT();

    _postFX->idle(_context.config());
}

void Renderer::updateResolution(const U16 newWidth, const U16 newHeight) const {
    _postFX->updateResolution(newWidth, newHeight);
}
}