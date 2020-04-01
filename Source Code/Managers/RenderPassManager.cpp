#include "stdafx.h"

#include "Headers/RenderPassManager.h"

#include "Core/Headers/Kernel.h"
#include "Core/Headers/TaskPool.h"
#include "Core/Headers/EngineTaskPool.h"
#include "Core/Headers/StringHelper.h"
#include "Core/Headers/Configuration.h"
#include "Core/Headers/PlatformContext.h"
#include "Core/Time/Headers/ProfileTimer.h"
#include "Managers/Headers/SceneManager.h"
#include "Rendering/Headers/Renderer.h"
#include "Rendering/Camera/Headers/Camera.h"
#include "Rendering/PostFX/Headers/PostFX.h"
#include "Rendering/RenderPass/Headers/RenderQueue.h"
#include "Platform/Video/Textures/Headers/Texture.h"
#include "Platform/Video/Buffers/ShaderBuffer/Headers/ShaderBuffer.h"

#include "ECS/Components/Headers/BoundsComponent.h"
#include "ECS/Components/Headers/RenderingComponent.h"
#include "ECS/Components/Headers/TransformComponent.h"
#include "ECS/Components/Headers/AnimationComponent.h"

namespace Divide {

    namespace {
        constexpr U32 g_nodesPerPrepareDrawPartition = 16u;
        constexpr bool g_singleThreadedCommandBufferCreation = false;

        struct PerPassData {
            RenderBin::SortedQueues sortedQueues;
            DrawCommandContainer drawCommands;

            std::array<GFXDevice::NodeData, Config::MAX_VISIBLE_NODES> nodeData;
            eastl::fixed_vector<RenderingComponent*, Config::MAX_VISIBLE_NODES, false> queuedRenderingComponents;
        };

        std::array<PerPassData, to_base(RenderStage::COUNT)> g_passData;
    };

    RenderPassManager::RenderPassManager(Kernel& parent, GFXDevice& context)
        : KernelComponent(parent),
          _renderQueue(parent),
          _context(context),
          _OITCompositionPipeline(nullptr),
          _postFxRenderTimer(&Time::ADD_TIMER("PostFX Timer")),
          _renderPassTimer(&Time::ADD_TIMER("RenderPasses Timer")),
          _buildCommandBufferTimer(&Time::ADD_TIMER("BuildCommandBuffers Timer")),
          _flushCommandBufferTimer(&Time::ADD_TIMER("FlushCommandBuffers Timer"))
    {
        _flushCommandBufferTimer->addChildTimer(*_buildCommandBufferTimer);

        ShaderModuleDescriptor vertModule = {};
        vertModule._moduleType = ShaderType::VERTEX;
        vertModule._sourceFile = "baseVertexShaders.glsl";
        vertModule._variant = "FullScreenQuad";

        ShaderModuleDescriptor fragModule = {};
        fragModule._moduleType = ShaderType::FRAGMENT;
        fragModule._sourceFile = "OITComposition.glsl";

        ShaderProgramDescriptor shaderDescriptor = {};
        shaderDescriptor._modules.push_back(vertModule);
        shaderDescriptor._modules.push_back(fragModule);

        ResourceDescriptor shaderResDesc("OITComposition");
        shaderResDesc.propertyDescriptor(shaderDescriptor);
        shaderResDesc.waitForReady(false);
        _OITCompositionShader = CreateResource<ShaderProgram>(parent.resourceCache(), shaderResDesc);

        _renderPassCommandBuffer.push_back(GFX::allocateCommandBuffer());
        _postFXCommandBuffer = GFX::allocateCommandBuffer(true);
    }

    RenderPassManager::~RenderPassManager()
    {
        for (GFX::CommandBuffer*& buf : _renderPassCommandBuffer) {
            GFX::deallocateCommandBuffer(buf);
        }

        GFX::deallocateCommandBuffer(_postFXCommandBuffer);

        MemoryManager::DELETE_CONTAINER(_renderPasses);
    }

    void RenderPassManager::postInit() {
        WAIT_FOR_CONDITION(_OITCompositionShader->getState() == ResourceState::RES_LOADED);

        PipelineDescriptor pipelineDescriptor;
        pipelineDescriptor._stateHash = _context.get2DStateBlock();
        pipelineDescriptor._shaderProgramHandle = _OITCompositionShader->getGUID();
        _OITCompositionPipeline = _context.newPipeline(pipelineDescriptor);
    }

    namespace {
        inline bool all_of(vectorEASTL<bool>::const_iterator first, vectorEASTL<bool>::const_iterator last, bool state)
        {
            for (; first != last; ++first) {
                if (*first != state) {
                    return false;
                }
            }
            return true;
        }
    };

    void RenderPassManager::render(SceneRenderState& sceneRenderState, Time::ProfileTimer* parentTimer) {
        OPTICK_EVENT();

        if (parentTimer != nullptr && !parentTimer->hasChildTimer(*_renderPassTimer)) {
            parentTimer->addChildTimer(*_renderPassTimer);
            parentTimer->addChildTimer(*_postFxRenderTimer);
            parentTimer->addChildTimer(*_flushCommandBufferTimer);
        }

        const Camera& cam = Attorney::SceneManagerRenderPass::playerCamera(parent().sceneManager());

        Attorney::SceneManagerRenderPass::preRenderAllPasses(parent().sceneManager(), cam);

        TaskPool& pool = parent().platformContext().taskPool(TaskPoolType::HIGH_PRIORITY);

        const U8 renderPassCount = to_U8(_renderPasses.size());

        Task* postFXTask = nullptr;

        {
            OPTICK_EVENT("RenderPassManager::BuildCommandBuffers");
            Time::ScopedTimer timeCommands(*_buildCommandBufferTimer);
            {
                Time::ScopedTimer timeAll(*_renderPassTimer);

                for (I8 i = 0; i < renderPassCount; ++i)
                { //All of our render passes should run in parallel

                    RenderPass* pass = _renderPasses[i];

                    GFX::CommandBuffer* buf = _renderPassCommandBuffer[i];
                    _renderTasks[i] = CreateTask(pool,
                                                 nullptr,
                                                 [pass, buf, &sceneRenderState](const Task & parentTask) {
                                                     OPTICK_EVENT("RenderPass: BuildCommandBuffer");
                                                     buf->clear(false);
                                                     pass->render(parentTask, sceneRenderState, *buf);
                                                     buf->batch();
                                                 },
                                                 false);
                    Start(*_renderTasks[i], g_singleThreadedCommandBufferCreation ? TaskPriority::REALTIME : TaskPriority::DONT_CARE);
                }
                { //PostFX should be pretty fast
                    GFX::CommandBuffer* buf = _postFXCommandBuffer;

                    Time::ProfileTimer& timer = *_postFxRenderTimer;
                    GFXDevice& gfx = parent().platformContext().gfx();
                    PostFX& postFX = gfx.getRenderer().postFX();

                    postFXTask = CreateTask(pool,
                                            nullptr,
                                            [buf, &gfx, &postFX, &cam, &timer](const Task & parentTask) {
                                                OPTICK_EVENT("PostFX: BuildCommandBuffer");

                                                buf->clear(false);

                                                Time::ScopedTimer time(timer);
                                                postFX.apply(cam, *buf);

                                                const Texture_ptr& srcTex = gfx.renderTargetPool().renderTarget(RenderTargetID(RenderTargetUsage::SCREEN)).getAttachment(RTAttachmentType::Depth, 0).texture();
                                                const Texture_ptr& dstTex = gfx.getPrevDepthBuffer();
                                                GFX::CopyTextureCommand copyCmd = {};
                                                copyCmd._source = srcTex->data();
                                                copyCmd._destination = dstTex->data();
                                                copyCmd._params._dimensions = {
                                                    dstTex->width(),
                                                    dstTex->height(),
                                                    dstTex->numLayers()
                                                };
                                                GFX::EnqueueCommand(*buf, copyCmd);

                                                buf->batch();
                                            });
                    Start(*postFXTask, g_singleThreadedCommandBufferCreation ? TaskPriority::REALTIME : TaskPriority::DONT_CARE);
                }
            }
        }
        {
            OPTICK_EVENT("RenderPassManager::FlushCommandBuffers");
            Time::ScopedTimer timeCommands(*_flushCommandBufferTimer);

            eastl::fill(eastl::begin(_completedPasses), eastl::end(_completedPasses), false);

            Task* whileRendering = CreateTask(pool, nullptr, [](const Task& parentTask) {
                //ToDo: Do other stuff here: some physx, some AI, some whatever.
                // This will be called in parallel to flushing a command buffer so be aware of any CPU AND GPU race conditions
            });
            ACKNOWLEDGE_UNUSED(whileRendering);

            bool slowIdle = false;
            while (!all_of(eastl::cbegin(_completedPasses), eastl::cend(_completedPasses), true)) {
                OPTICK_EVENT("ON_LOOP");

                // For every render pass
                bool finished = true;
                for (U8 i = 0; i < renderPassCount; ++i) {
                    if (_completedPasses[i] || !Finished(*_renderTasks[i])) {
                        continue;
                    }

                    // Grab the list of dependencies
                    const vectorSTD<U8>& dependencies = _renderPasses[i]->dependencies();

                    bool dependenciesRunning = false;
                    // For every dependency in the list
                    for (const U8 dep : dependencies) {
                        if (dependenciesRunning) {
                            break;
                        }

                        // Try and see if it's running
                        for (U8 j = 0; j < renderPassCount; ++j) {
                            // If it is running, we can't render yet
                            if (j != i && _renderPasses[j]->sortKey() == dep && !_completedPasses[j]) {
                                dependenciesRunning = true;
                                break;
                            }
                        }
                    }

                    if (!dependenciesRunning) {
                        OPTICK_TAG("Buffer ID: ", i);
                        //Start(*whileRendering);
                        // No running dependency so we can flush the command buffer and add the pass to the skip list
                        _context.flushCommandBuffer(*_renderPassCommandBuffer[i], false);
                        _completedPasses[i] = true;
                        //Wait(*whileRendering);

                    } else {
                        finished = false;
                    }
                }

                if (!finished) {
                    OPTICK_EVENT("IDLING");

                    parent().idle(!slowIdle);
                    std::this_thread::yield();
                    slowIdle = !slowIdle;
                }
            }
        }

        // Flush the postFX stack
        Wait(*postFXTask);
        _context.flushCommandBuffer(*_postFXCommandBuffer, false);

        for (U8 i = 0; i < renderPassCount; ++i) {
            _renderPasses[i]->postRender();
        }

        Attorney::SceneManagerRenderPass::postRenderAllPasses(parent().sceneManager(), cam);
    }

RenderPass& RenderPassManager::addRenderPass(const Str64& renderPassName,
                                             U8 orderKey,
                                             RenderStage renderStage,
                                             vectorSTD<U8> dependencies,
                                             bool usePerformanceCounters) {
    assert(!renderPassName.empty());

    _renderPasses.push_back(MemoryManager_NEW RenderPass(*this, _context, renderPassName, orderKey, renderStage, dependencies, usePerformanceCounters));
    RenderPass* item = _renderPasses.back();

    item->initBufferData();

    //Secondary command buffers. Used in a threaded fashion
    _renderPassCommandBuffer.push_back(GFX::allocateCommandBuffer(true));

    eastl::sort(eastl::begin(_renderPasses), eastl::end(_renderPasses), [](RenderPass* a, RenderPass* b) -> bool { return a->sortKey() < b->sortKey(); });

    _renderTasks.resize(_renderPasses.size());
    _completedPasses.resize(_renderPasses.size(), false);

    return *item;
}

void RenderPassManager::removeRenderPass(const Str64& name) {
    for (vectorEASTL<RenderPass*>::iterator it = eastl::begin(_renderPasses); it != eastl::end(_renderPasses); ++it) {
         if ((*it)->name().compare(name) == 0) {
            _renderPasses.erase(it);
            // Remove one command buffer
            GFX::CommandBuffer* buf = _renderPassCommandBuffer.back();
            GFX::deallocateCommandBuffer(buf, true);
            _renderPassCommandBuffer.pop_back();
            break;
        }
    }

    _renderTasks.resize(_renderPasses.size());
    _completedPasses.resize(_renderPasses.size(), false);
}

U32 RenderPassManager::getLastTotalBinSize(RenderStage renderStage) const {
    return getPassForStage(renderStage).getLastTotalBinSize();
}

RenderPass& RenderPassManager::getPassForStage(RenderStage renderStage) {
    for (RenderPass* pass : _renderPasses) {
        if (pass->stageFlag() == renderStage) {
            return *pass;
        }
    }

    DIVIDE_UNEXPECTED_CALL();
    return *_renderPasses.front();
}

const RenderPass& RenderPassManager::getPassForStage(RenderStage renderStage) const {
    for (RenderPass* pass : _renderPasses) {
        if (pass->stageFlag() == renderStage) {
            return *pass;
        }
    }

    DIVIDE_UNEXPECTED_CALL();
    return *_renderPasses.front();
}

RenderPass::BufferData RenderPassManager::getBufferData(const RenderStagePass& stagePass) const {
    return getPassForStage(stagePass._stage).getBufferData(stagePass);
}

/// Prepare the list of visible nodes for rendering
void RenderPassManager::processVisibleNode(const RenderingComponent& rComp, const RenderStagePass& stagePass, bool playAnimations, const mat4<F32>& viewMatrix, const D64 interpolationFactor, bool needsInterp, GFXDevice::NodeData& dataOut) const {
    OPTICK_EVENT();

    const SceneGraphNode& node = rComp.getSGN();
    const TransformComponent* const transform = node.get<TransformComponent>();
    assert(transform != nullptr);

    // ... get the node's world matrix properly interpolated
    if (needsInterp) {
        transform->getWorldMatrix(interpolationFactor, dataOut._worldMatrix);
    } else {
        dataOut._worldMatrix.set(transform->getWorldMatrix());
    }

    dataOut._normalMatrixW.set(dataOut._worldMatrix);
    if (!transform->isUniformScaled()) {
        // Non-uniform scaling requires an inverseTranspose to negate
        // scaling contribution but preserve rotation
        dataOut._normalMatrixW.setRow(3, 0.0f, 0.0f, 0.0f, 1.0f);
        dataOut._normalMatrixW.inverseTranspose();
    }

    // Get the material property matrix (alpha test, texture count, texture operation, etc.)
    if (playAnimations) {
        AnimationComponent* animComp = node.get<AnimationComponent>();
        if (animComp && animComp->playAnimations()) {
            dataOut._normalMatrixW.element(0, 3) = to_F32(animComp->boneCount());
        }
    }

    // Since the normal matrix is 3x3, we can use the extra row and column to store additional data
    const BoundsComponent* const bounds = node.get<BoundsComponent>();
    const BoundingBox& aabb = bounds->getBoundingBox();
    const F32 sphereRadius = bounds->getBoundingSphere().getRadius();

    dataOut._normalMatrixW.setRow(3, vec4<F32>(aabb.getCenter(), sphereRadius));
    // Get the colour matrix (diffuse, specular, etc.)
    rComp.getMaterialColourMatrix(dataOut._colourMatrix);

    RenderingComponent::NodeRenderingProperties properties = {};
    rComp.getRenderingProperties(stagePass, properties);

    dataOut._normalMatrixW.element(1, 3) = to_F32(properties._reflectionIndex);
    dataOut._normalMatrixW.element(2, 3) = to_F32(properties._refractionIndex);
    dataOut._colourMatrix.element(2, 3) = properties._receivesShadows ? 1.0f : 0.0f;
    dataOut._colourMatrix.element(3, 2) = to_F32(properties._lod);
    //set properties.w to negative value to skip occlusion culling for the node
    dataOut._colourMatrix.element(3, 3) = properties._cullFlagValue;

    // Temp: Make the hovered/selected node brighter by setting emissive to something bright
    if (properties._isSelected || properties._isHovered) {
        FColour4 matColour = dataOut._colourMatrix.getRow(2);
        if (properties._isSelected) {
            matColour.rgb({0.f, 0.50f, 0.f});
        } else {
            matColour.rgb({0.f, 0.25f, 0.f});
        }
        dataOut._colourMatrix.setRow(2, matColour);
    }

    dataOut._extraProperties.x = to_F32(properties._texOperation);
    dataOut._extraProperties.y = to_F32(properties._bumpMethod);
}

void RenderPassManager::buildBufferData(const RenderStagePass& stagePass,
                                        const SceneRenderState& renderState,
                                        const Camera& camera,
                                        const RenderBin::SortedQueues& sortedQueues,
                                        bool fullRefresh,
                                        GFX::CommandBuffer& bufferInOut)
{
    OPTICK_EVENT();
    OPTICK_TAG("FULL_REFRESH", fullRefresh);

    PerPassData& passData = g_passData[to_base(stagePass._stage)];
    RefreshNodeDataParams params = {};
    params._drawCommandsInOut = &passData.drawCommands;
    params._bufferInOut = &bufferInOut;
    params._camera = &camera;
    params._stagePass = &stagePass;

    if (fullRefresh) {
        params._drawCommandsInOut->clear();

        const mat4<F32>& viewMatrix = camera.getViewMatrix();
        const D64 interpFactor = _context.getFrameInterpolationFactor();
        const bool needsInterp = interpFactor < 0.985;

        U32 dataIdx = 0u;
        const bool playAnimations = renderState.isEnabledOption(SceneRenderState::RenderOptions::PLAY_ANIMATIONS);

        for(RenderingComponent * rComp : passData.queuedRenderingComponents) {
            if (Attorney::RenderingCompRenderPass::onRefreshNodeData(*rComp, params, dataIdx)) {
                GFXDevice::NodeData& data = passData.nodeData[dataIdx];
                processVisibleNode(*rComp, stagePass, playAnimations, viewMatrix, interpFactor, needsInterp, data);
                ++dataIdx;
            }
        }

        RenderPass::BufferData bufferData = getPassForStage(stagePass._stage).getBufferData(stagePass);
        *bufferData._lastCommandCount = to_U32(passData.drawCommands.size());
        *bufferData._lastNodeCount = to_U32(passData.queuedRenderingComponents.size());

        {
            OPTICK_EVENT("RenderPassManager::buildBufferData - UpdateBuffers");
            bufferData._nodeData->writeData(
                bufferData._elementOffset,
                *bufferData._lastNodeCount,
                passData.nodeData.data());

            bufferData._cmdBuffer->writeData(
                bufferData._elementOffset,
                *bufferData._lastCommandCount,
                passData.drawCommands.data());
        }

        ShaderBufferBinding cmdBuffer = {};
        cmdBuffer._binding = ShaderBufferLocation::CMD_BUFFER;
        cmdBuffer._buffer = bufferData._cmdBuffer;
        cmdBuffer._elementRange = { bufferData._elementOffset, *bufferData._lastCommandCount };

        ShaderBufferBinding dataBuffer = {};
        dataBuffer._binding = ShaderBufferLocation::NODE_INFO;
        dataBuffer._buffer = bufferData._nodeData;
        dataBuffer._elementRange = { bufferData._elementOffset, *bufferData._lastNodeCount };

        GFX::BindDescriptorSetsCommand descriptorSetCmd = {};
        descriptorSetCmd._set.addShaderBuffer(cmdBuffer);
        descriptorSetCmd._set.addShaderBuffer(dataBuffer);
        GFX::EnqueueCommand(bufferInOut, descriptorSetCmd);
    } else {
        for (RenderingComponent* rComp : passData.queuedRenderingComponents) {
            Attorney::RenderingCompRenderPass::onQuickRefreshNodeData(*rComp, params);
        }
    }
}

void RenderPassManager::buildDrawCommands(const PassParams& params, bool refresh, GFX::CommandBuffer& bufferInOut)
{
    OPTICK_EVENT();

    const RenderStagePass& stagePass = params._stagePass;
    PerPassData& passData = g_passData[to_base(stagePass._stage)];

    const SceneRenderState& sceneRenderState = parent().sceneManager().getActiveScene().renderState();

    U16 queueSize = 0;
    for (RenderBin::SortedQueue& queue : passData.sortedQueues) {
        queue.resize(0);
        queue.reserve(Config::MAX_VISIBLE_NODES);
    }

    getQueue().getSortedQueues(stagePass._stage, stagePass._passType, passData.sortedQueues, queueSize);

    auto& rComps = passData.queuedRenderingComponents;
    rComps.resize(0);

    if (params._sourceNode != nullptr) {
        const I64 sourceGUID = params._sourceNode->getGUID();

        for (const RenderBin::SortedQueue& queue : passData.sortedQueues) {
            for (RenderingComponent* entry : queue) {
                if (sourceGUID != entry->getSGN().getGUID()) {
                    rComps.push_back(entry);
                }
            }
        }
    } else {
        for (const RenderBin::SortedQueue& queue : passData.sortedQueues) {
            rComps.insert(eastl::cend(rComps),
                          eastl::cbegin(queue),
                          eastl::cend(queue));
        }
    }

    const Camera& cam = *params._camera;
 
    ParallelForDescriptor descriptor = {};
    descriptor._iterCount = to_U32(rComps.size());
    descriptor._partitionSize = g_nodesPerPrepareDrawPartition;
    descriptor._priority = g_singleThreadedCommandBufferCreation ? TaskPriority::DONT_CARE : TaskPriority::REALTIME;
    descriptor._useCurrentThread = true;

    parallel_for(_parent.platformContext(),
        [&rComps, &cam, &sceneRenderState, &stagePass, refresh](const Task* parentTask, U32 start, U32 end) {
            for (U32 i = start; i < end; ++i) {
                Attorney::RenderingCompRenderPass::prepareDrawPackage(*rComps[i], cam, sceneRenderState, stagePass, refresh);
            }
        },
        descriptor);
    
    buildBufferData(stagePass, sceneRenderState, *params._camera, passData.sortedQueues, refresh, bufferInOut);
}

void RenderPassManager::prepareRenderQueues(const PassParams& params, const VisibleNodeList& nodes, bool refreshNodeData, GFX::CommandBuffer& bufferInOut) {
    OPTICK_EVENT();
    const RenderStagePass& stagePass = params._stagePass;

    RenderQueue& queue = getQueue();
    queue.refresh(stagePass._stage);
    for (const VisibleNode& node : nodes) {
        queue.addNodeToQueue(*node._node, stagePass, node._distanceToCameraSq);
    }
    // Sort all bins
    queue.sort(stagePass);
    
    vectorEASTLFast<RenderPackage*>& packageQueue = _renderQueues[to_base(stagePass._stage)];
    packageQueue.resize(0);
    packageQueue.reserve(Config::MAX_VISIBLE_NODES);
    
    // Draw everything in the depth pass
    if (stagePass.isDepthPass()) {
        queue.populateRenderQueues(stagePass, std::make_pair(RenderBinType::RBT_COUNT, true), packageQueue);
    } else {
        // Only draw stuff from the translucent bin in the OIT Pass and everything else in the colour pass
        bool oitPass = stagePass._passType == RenderPassType::OIT_PASS;
        queue.populateRenderQueues(stagePass, std::make_pair(RenderBinType::RBT_TRANSLUCENT, oitPass), packageQueue);
    }

    buildDrawCommands(params, refreshNodeData, bufferInOut);
}

bool RenderPassManager::prePass(const VisibleNodeList& nodes, const PassParams& params, const RenderTarget& target, GFX::CommandBuffer& bufferInOut) {
    OPTICK_EVENT();

    assert(params._stagePass._passType == RenderPassType::PRE_PASS);

    // PrePass requires a depth buffer
    const bool doPrePass = params._stagePass._stage != RenderStage::SHADOW &&
                           params._target._usage != RenderTargetUsage::COUNT &&
                           target.getAttachment(RTAttachmentType::Depth, 0).used();

    if (doPrePass) {
        GFX::BeginDebugScopeCommand beginDebugScopeCmd = {};
        beginDebugScopeCmd._scopeID = 0;
        beginDebugScopeCmd._scopeName = " - PrePass";
        GFX::EnqueueCommand(bufferInOut, beginDebugScopeCmd);

        prepareRenderQueues(params, nodes, true, bufferInOut);

        RTDrawDescriptor normalsAndDepthPolicy = {};
        normalsAndDepthPolicy.drawMask().disableAll();
        normalsAndDepthPolicy.drawMask().setEnabled(RTAttachmentType::Depth, 0, true);
        normalsAndDepthPolicy.drawMask().setEnabled(RTAttachmentType::Colour, to_base(GFXDevice::ScreenTargets::EXTRA), true);
        normalsAndDepthPolicy.drawMask().setEnabled(RTAttachmentType::Colour, to_base(GFXDevice::ScreenTargets::NORMALS_AND_VELOCITY), true);

        if (params._bindTargets) {
            GFX::BeginRenderPassCommand beginRenderPassCommand;
            beginRenderPassCommand._target = params._target;
            beginRenderPassCommand._descriptor = normalsAndDepthPolicy;
            beginRenderPassCommand._name = "DO_PRE_PASS";
            GFX::EnqueueCommand(bufferInOut, beginRenderPassCommand);
        }

        renderQueueToSubPasses(params._stagePass._stage, bufferInOut);

        Attorney::SceneManagerRenderPass::postRender(parent().sceneManager(), params._stagePass, *params._camera, bufferInOut);

        if (params._bindTargets) {
            GFX::EnqueueCommand(bufferInOut, GFX::EndRenderPassCommand{});
        }

        GFX::EnqueueCommand(bufferInOut, GFX::EndDebugScopeCommand{});
    }

    return doPrePass;
}

bool RenderPassManager::occlusionPass(const VisibleNodeList& nodes, const PassParams& params, vec2<bool> extraTargets, const RenderTarget& target, bool prePassExecuted, GFX::CommandBuffer& bufferInOut) {
    OPTICK_EVENT();

    ACKNOWLEDGE_UNUSED(nodes);
    ACKNOWLEDGE_UNUSED(extraTargets);

    const RenderStagePass& stagePass = params._stagePass;
    assert(stagePass._passType == RenderPassType::PRE_PASS);

    if (params._targetHIZ._usage != RenderTargetUsage::COUNT) {
        GFX::ResolveRenderTargetCommand resolveCmd = { };
        resolveCmd._source = params._target;
        resolveCmd._resolveDepth = true;
        GFX::EnqueueCommand(bufferInOut, resolveCmd);

        // Update HiZ Target
        const Texture_ptr& HiZTex = _context.constructHIZ(params._target, params._targetHIZ, bufferInOut);

        // ToDo: This should not be needed as we unbind the render target before we dispatch the compute task anyway.
        // See if we can remove this -Ionut
        GFX::MemoryBarrierCommand memCmd = {};
        memCmd._barrierMask = to_base(MemoryBarrierType::RENDER_TARGET) | 
                              to_base(MemoryBarrierType::TEXTURE_FETCH) | 
                              to_base(MemoryBarrierType::TEXTURE_BARRIER);
        GFX::EnqueueCommand(bufferInOut, memCmd);

        // Run occlusion culling CS
        const RenderPass::BufferData& bufferData = getBufferData(stagePass);
        _context.occlusionCull(bufferData, HiZTex, *params._camera, bufferInOut);

        // Occlusion culling barrier
        memCmd._barrierMask = to_base(MemoryBarrierType::COMMAND_BUFFER) | //For rendering
                              to_base(MemoryBarrierType::SHADER_STORAGE);  //For updating later on

        if (bufferData._cullCounter != nullptr) {
            memCmd._barrierMask |= to_base(MemoryBarrierType::ATOMIC_COUNTER);
            GFX::EnqueueCommand(bufferInOut, memCmd);

            _context.updateCullCount(bufferData, bufferInOut);

            bufferData._cullCounter->incQueue();

            GFX::ClearBufferDataCommand clearAtomicCounter;
            clearAtomicCounter._buffer = bufferData._cullCounter;
            clearAtomicCounter._offsetElementCount = 0;
            clearAtomicCounter._elementCount = 1;
            GFX::EnqueueCommand(bufferInOut, clearAtomicCounter);
        } else {
            GFX::EnqueueCommand(bufferInOut, memCmd);
        }

        return true;
    }
     
    return false; // no HIZ!
}

void RenderPassManager::mainPass(const VisibleNodeList& nodes, const PassParams& params, vec2<bool> extraTargets, RenderTarget& target, bool prePassExecuted, bool hasHiZ, GFX::CommandBuffer& bufferInOut) {
    OPTICK_EVENT();

    const RenderStagePass& stagePass = params._stagePass;
    assert(stagePass._passType == RenderPassType::MAIN_PASS);

    GFX::BeginDebugScopeCommand beginDebugScopeCmd = {};
    beginDebugScopeCmd._scopeID = 1;
    beginDebugScopeCmd._scopeName = " - MainPass";
    GFX::EnqueueCommand(bufferInOut, beginDebugScopeCmd);

    prepareRenderQueues(params, nodes, !prePassExecuted, bufferInOut);

    if (params._target._usage != RenderTargetUsage::COUNT) {
        SceneManager& sceneManager = parent().sceneManager();

        Texture_ptr hizTex = nullptr;
        Texture_ptr depthTex = nullptr;
        if (hasHiZ) {
            const RenderTarget& hizTarget = _context.renderTargetPool().renderTarget(params._targetHIZ);
            hizTex = hizTarget.getAttachment(RTAttachmentType::Depth, 0).texture();
        }
        if (prePassExecuted) {
            depthTex = target.getAttachment(RTAttachmentType::Depth, 0).texture();
        }

        Attorney::SceneManagerRenderPass::preRenderMainPass(sceneManager, stagePass, *params._camera, hizTex, bufferInOut);

        const bool hasNormalsTarget = extraTargets.x;
        const bool hasLightingTarget = extraTargets.y;

        if (params._bindTargets) {
            // We don't need to clear the colour buffers at this stage since ... hopefully, they will be overwritten completely. Right?
            RTDrawDescriptor drawPolicy = {};
            if (params._drawPolicy != nullptr) {
                drawPolicy = *params._drawPolicy;
            }

            GFX::BindDescriptorSetsCommand descriptorSetCmd = {};
            if (hasNormalsTarget) {
                GFX::ResolveRenderTargetCommand resolveCmd = { };
                resolveCmd._source = params._target;
                resolveCmd._resolveColour = to_I8(GFXDevice::ScreenTargets::NORMALS_AND_VELOCITY);
                GFX::EnqueueCommand(bufferInOut, resolveCmd);

                const TextureData& data = target.getAttachmentPtr(RTAttachmentType::Colour, to_U8(GFXDevice::ScreenTargets::NORMALS_AND_VELOCITY))->texture()->data();
                constexpr U8 bindSlot = to_U8(ShaderProgram::TextureUsage::NORMALMAP);
                descriptorSetCmd._set._textureData.setTexture(data, bindSlot);
            }

            if (prePassExecuted) {
                drawPolicy.drawMask().setEnabled(RTAttachmentType::Depth, 0, false);
                drawPolicy.drawMask().setEnabled(RTAttachmentType::Colour, to_U8(GFXDevice::ScreenTargets::NORMALS_AND_VELOCITY), false);
                drawPolicy.drawMask().setEnabled(RTAttachmentType::Colour, to_U8(GFXDevice::ScreenTargets::EXTRA), false);
                const TextureData depthData = hasHiZ ? hizTex->data() : depthTex->data();
                descriptorSetCmd._set._textureData.setTexture(depthData, to_base(ShaderProgram::TextureUsage::DEPTH));
            }
            if (hasLightingTarget) {
                GFX::ResolveRenderTargetCommand resolveCmd = { };
                resolveCmd._source = params._target;
                resolveCmd._resolveColour = to_I8(GFXDevice::ScreenTargets::EXTRA);
                GFX::EnqueueCommand(bufferInOut, resolveCmd);

                const TextureData& data = target.getAttachmentPtr(RTAttachmentType::Colour, to_U8(GFXDevice::ScreenTargets::EXTRA))->texture()->data();
                constexpr U8 bindSlot = to_U8(ShaderProgram::TextureUsage::GBUFFER_EXTRA);

                descriptorSetCmd._set._textureData.setTexture(data, bindSlot);
            }

            GFX::BeginRenderPassCommand beginRenderPassCommand;
            beginRenderPassCommand._target = params._target;
            beginRenderPassCommand._descriptor = drawPolicy;
            beginRenderPassCommand._name = "DO_MAIN_PASS";
            GFX::EnqueueCommand(bufferInOut, beginRenderPassCommand);

            GFX::EnqueueCommand(bufferInOut, descriptorSetCmd);
        }

        // We try and render translucent items in the shadow pass and due some alpha-discard tricks
        renderQueueToSubPasses(stagePass._stage, bufferInOut);

        Attorney::SceneManagerRenderPass::postRender(sceneManager, stagePass, *params._camera, bufferInOut);

        if (stagePass._stage == RenderStage::DISPLAY) {
            /// These should be OIT rendered as well since things like debug nav meshes have translucency
            Attorney::SceneManagerRenderPass::debugDraw(sceneManager, stagePass, *params._camera, bufferInOut);
        }

        if (params._bindTargets) {
            GFX::EnqueueCommand(bufferInOut, GFX::EndRenderPassCommand{});
        }
    }

    GFX::EnqueueCommand(bufferInOut, GFX::EndDebugScopeCommand{});
}

void RenderPassManager::woitPass(const VisibleNodeList& nodes, const PassParams& params, vec2<bool> extraTargets, const RenderTarget& target, GFX::CommandBuffer& bufferInOut) {
    OPTICK_EVENT();

    const RenderStagePass& stagePass = params._stagePass;
    assert(stagePass._passType == RenderPassType::OIT_PASS);

    prepareRenderQueues(params, nodes, false, bufferInOut);

    GFX::BeginDebugScopeCommand beginDebugScopeCmd = {};
    beginDebugScopeCmd._scopeID = 2;
    beginDebugScopeCmd._scopeName = " - W-OIT Pass";
    GFX::EnqueueCommand(bufferInOut, beginDebugScopeCmd);

    if (renderQueueSize(stagePass._stage) > 0) {
        GFX::ClearRenderTargetCommand clearRTCmd = {};
        clearRTCmd._target = RenderTargetID(RenderTargetUsage::OIT);
        if (Config::USE_COLOURED_WOIT) {
            // Don't clear our screen target. That would be BAD.
            clearRTCmd._descriptor.clearColour(to_U8(GFXDevice::ScreenTargets::MODULATE), false);
        }
        // Don't clear and don't write to depth buffer
        clearRTCmd._descriptor.clearDepth(false);
        GFX::EnqueueCommand(bufferInOut, clearRTCmd);

        // Step1: Draw translucent items into the accumulation and revealage buffers
        GFX::BeginRenderPassCommand beginRenderPassOitCmd;
        beginRenderPassOitCmd._name = "DO_OIT_PASS_1";
        beginRenderPassOitCmd._target = RenderTargetID(RenderTargetUsage::OIT);
        {
            RTBlendState& state0 = beginRenderPassOitCmd._descriptor.blendState(to_U8(GFXDevice::ScreenTargets::ACCUMULATION));
            state0._blendProperties._enabled = true;
            state0._blendProperties._blendSrc = BlendProperty::ONE;
            state0._blendProperties._blendDest = BlendProperty::ONE;
            state0._blendProperties._blendOp = BlendOperation::ADD;

            RTBlendState& state1 = beginRenderPassOitCmd._descriptor.blendState(to_U8(GFXDevice::ScreenTargets::REVEALAGE));
            state1._blendProperties._enabled = true;
            state1._blendProperties._blendSrc = BlendProperty::ZERO;
            state1._blendProperties._blendDest = BlendProperty::INV_SRC_COLOR;
            state1._blendProperties._blendOp = BlendOperation::ADD;

            if (Config::USE_COLOURED_WOIT) {
                RTBlendState& state2 = beginRenderPassOitCmd._descriptor.blendState(to_U8(GFXDevice::ScreenTargets::EXTRA));
                state2._blendProperties._enabled = true;
                state2._blendProperties._blendSrc = BlendProperty::ONE;
                state2._blendProperties._blendDest = BlendProperty::ONE;
                state2._blendProperties._blendOp = BlendOperation::ADD;
            }
        }

        beginRenderPassOitCmd._descriptor.drawMask().setEnabled(RTAttachmentType::Depth, 0, false);
        GFX::EnqueueCommand(bufferInOut, beginRenderPassOitCmd);

        renderQueueToSubPasses(stagePass._stage, bufferInOut/*, quality*/);

        GFX::EndRenderPassCommand endRenderPassOitCmd;
        endRenderPassOitCmd._autoResolveMSAAColour = true;
        endRenderPassOitCmd._autoResolveMSAAExternalColour = false;
        endRenderPassOitCmd._autoResolveMSAADepth = false;
        GFX::EnqueueCommand(bufferInOut, endRenderPassOitCmd);

        // Step2: Composition pass
        // Don't clear depth & colours and do not write to the depth buffer

        GFX::EnqueueCommand(bufferInOut, GFX::SetCameraCommand{ Camera::utilityCamera(Camera::UtilityCamera::_2D)->snapshot() });

        GFX::BeginRenderPassCommand beginRenderPassCompCmd;
        beginRenderPassCompCmd._name = "DO_OIT_PASS_2";
        beginRenderPassCompCmd._target = params._target;
        beginRenderPassCompCmd._descriptor.drawMask().setEnabled(RTAttachmentType::Depth, 0, false);
        {
            RTBlendState& state0 = beginRenderPassCompCmd._descriptor.blendState(to_U8(GFXDevice::ScreenTargets::ALBEDO));
            state0._blendProperties._enabled = true;
            state0._blendProperties._blendOp = BlendOperation::ADD;
            if (Config::USE_COLOURED_WOIT) {
                state0._blendProperties._blendSrc = BlendProperty::INV_SRC_ALPHA;
                state0._blendProperties._blendDest = BlendProperty::ONE;
            } else {
                state0._blendProperties._blendSrc = BlendProperty::SRC_ALPHA;
                state0._blendProperties._blendDest = BlendProperty::INV_SRC_ALPHA;
            }
        }
        GFX::EnqueueCommand(bufferInOut, beginRenderPassCompCmd);

        GFX::EnqueueCommand(bufferInOut, GFX::BindPipelineCommand{ _OITCompositionPipeline });

        RenderTarget& oitTarget = _context.renderTargetPool().renderTarget(RenderTargetID(RenderTargetUsage::OIT));
        TextureData accum = oitTarget.getAttachment(RTAttachmentType::Colour, to_U8(GFXDevice::ScreenTargets::ACCUMULATION)).texture()->data();
        TextureData revealage = oitTarget.getAttachment(RTAttachmentType::Colour, to_U8(GFXDevice::ScreenTargets::REVEALAGE)).texture()->data();

        GFX::BindDescriptorSetsCommand descriptorSetCmd = {};
        descriptorSetCmd._set._textureData.setTextures(
            {
                { to_base(ShaderProgram::TextureUsage::UNIT0), accum },
                { to_base(ShaderProgram::TextureUsage::UNIT1), revealage }
            }
        );
        GFX::EnqueueCommand(bufferInOut, descriptorSetCmd);

        GenericDrawCommand drawCommand;
        drawCommand._primitiveType = PrimitiveType::TRIANGLES;

        GFX::EnqueueCommand(bufferInOut, GFX::DrawCommand{ drawCommand });

        GFX::EndRenderPassCommand endRenderPassCompCmd;
        endRenderPassCompCmd._autoResolveMSAAColour = true;
        GFX::EnqueueCommand(bufferInOut, endRenderPassCompCmd);
    }

    GFX::EnqueueCommand(bufferInOut, GFX::EndDebugScopeCommand{});
}

void RenderPassManager::doCustomPass(PassParams params, GFX::CommandBuffer& bufferInOut) {
    OPTICK_EVENT();

    const RenderStage stage = params._stagePass._stage;

    GFX::BeginDebugScopeCommand beginDebugScopeCmd = {};
    beginDebugScopeCmd._scopeID = 0;
    beginDebugScopeCmd._scopeName = Util::StringFormat("Custom pass ( %s - %s )", TypeUtil::RenderStageToString(stage), params._passName.empty() ? "N/A" : params._passName.c_str()).c_str();
    GFX::EnqueueCommand(bufferInOut, beginDebugScopeCmd);

    Attorney::SceneManagerRenderPass::prepareLightData(parent().sceneManager(), stage, *params._camera);

    // Cull the scene and grab the visible nodes
    const VisibleNodeList& visibleNodes = Attorney::SceneManagerRenderPass::cullScene(parent().sceneManager(), stage, *params._camera, params._minLoD, params._minExtents);

    // Tell the Rendering API to draw from our desired PoV
    GFX::EnqueueCommand(bufferInOut, GFX::SetCameraCommand{ params._camera->snapshot() });
    GFX::EnqueueCommand(bufferInOut, GFX::SetClipPlanesCommand{ params._clippingPlanes });

    RenderTarget& target = _context.renderTargetPool().renderTarget(params._target);

    const vec2<bool> extraTargets = {
        target.hasAttachment(RTAttachmentType::Colour, to_base(GFXDevice::ScreenTargets::NORMALS_AND_VELOCITY)),
        target.hasAttachment(RTAttachmentType::Colour, to_base(GFXDevice::ScreenTargets::EXTRA))
    };

    if (params._bindTargets) {
        RTClearDescriptor clearDescriptor = {};
        if (params._clearDescriptor != nullptr) {
            clearDescriptor = *params._clearDescriptor;
        }

        GFX::ClearRenderTargetCommand clearMainTarget = {};
        clearMainTarget._target = params._target;
        clearMainTarget._descriptor = clearDescriptor;
        GFX::EnqueueCommand(bufferInOut, clearMainTarget);
    }

    GFX::BindDescriptorSetsCommand bindDescriptorSets = {};
    bindDescriptorSets._set._textureData.setTexture(_context.getPrevDepthBuffer()->data(), to_U8(ShaderProgram::TextureUsage::DEPTH_PREV));
    GFX::EnqueueCommand(bufferInOut, bindDescriptorSets);

    params._stagePass._passType = RenderPassType::PRE_PASS;
    const bool prePassExecuted = prePass(visibleNodes, params, target, bufferInOut);

    const bool hasHiZ = prePassExecuted && 
                        occlusionPass(visibleNodes, params, extraTargets, target, prePassExecuted, bufferInOut);

    if (stage == RenderStage::DISPLAY) {
        //ToDo: Might be worth having pre-pass operations per render stage, but currently, only the main pass needs SSAO, bloom and so forth
        _context.getRenderer().postFX().prepare(*params._camera, bufferInOut);
    }

    params._stagePass._passType = RenderPassType::MAIN_PASS;
    mainPass(visibleNodes, params, extraTargets, target, prePassExecuted, hasHiZ, bufferInOut);

    if (stage != RenderStage::SHADOW) {

        GFX::ResolveRenderTargetCommand resolveCmd = {};
        resolveCmd._source = params._target;
        resolveCmd._resolveColours = true;
        GFX::EnqueueCommand(bufferInOut, resolveCmd);

        params._stagePass._passType = RenderPassType::OIT_PASS;
        woitPass(visibleNodes, params, extraTargets, target, bufferInOut);
    }

    GFX::EnqueueCommand(bufferInOut, GFX::EndDebugScopeCommand{});
}

void RenderPassManager::createFrameBuffer(const Rect<I32>& targetViewport, GFX::CommandBuffer& bufferInOut) {
    PlatformContext& context = parent().platformContext();
    GFXDevice& gfx = _context;
    const GFXRTPool& pool = gfx.renderTargetPool();
    const RenderTarget& screen = pool.renderTarget(RenderTargetID(RenderTargetUsage::SCREEN));
    const TextureData texData = screen.getAttachment(RTAttachmentType::Colour, to_U8(GFXDevice::ScreenTargets::ALBEDO)).texture()->data();

    GFX::BeginDebugScopeCommand beginDebugScopeCmd = {};
    beginDebugScopeCmd._scopeID = 12345;
    beginDebugScopeCmd._scopeName = "Flush Display";
    GFX::EnqueueCommand(bufferInOut, beginDebugScopeCmd);

    GFX::ResolveRenderTargetCommand resolveCmd = { };
    resolveCmd._source = RenderTargetID(RenderTargetUsage::SCREEN);
    resolveCmd._resolveColours = true;
    resolveCmd._resolveDepth = false;
    GFX::EnqueueCommand(bufferInOut, resolveCmd);

    gfx.drawTextureInViewport(texData, targetViewport, true, false, bufferInOut);
    Attorney::SceneManagerRenderPass::drawCustomUI(_parent.sceneManager(), targetViewport, bufferInOut);
    context.gui().draw(gfx, targetViewport, bufferInOut);
    gfx.renderDebugUI(targetViewport, bufferInOut);

    GFX::EndDebugScopeCommand endDebugScopeCommand = {};
    GFX::EnqueueCommand(bufferInOut, endDebugScopeCommand);
}

// TEMP
U32 RenderPassManager::renderQueueSize(RenderStage stage, RenderPackage::MinQuality qualityRequirement) const {
    const vectorEASTLFast<RenderPackage*>& queue = _renderQueues[to_base(stage)];
    if (qualityRequirement == RenderPackage::MinQuality::COUNT) {
        return to_U32(queue.size());
    }

    U32 size = 0;
    for (const RenderPackage* item : queue) {
        if (item->qualityRequirement() == qualityRequirement) {
            ++size;
        }
    }

    return size;
}

void RenderPassManager::renderQueueToSubPasses(RenderStage stage, GFX::CommandBuffer& commandsInOut, RenderPackage::MinQuality qualityRequirement) const {
    OPTICK_EVENT();

    const vectorEASTLFast<RenderPackage*>& queue = _renderQueues[to_base(stage)];

    eastl::fixed_vector<GFX::CommandBuffer*, Config::MAX_VISIBLE_NODES> buffers = {};

    if (qualityRequirement == RenderPackage::MinQuality::COUNT) {
        for (RenderPackage* item : queue) {
            buffers.push_back(Attorney::RenderPackageRenderPassManager::getCommandBuffer(*item));
        }
    } else {
        for (RenderPackage* item : queue) {
            if (item->qualityRequirement() == qualityRequirement) {
                buffers.push_back(Attorney::RenderPackageRenderPassManager::getCommandBuffer(*item));
            }
        }
    }

    commandsInOut.add(buffers.data(), buffers.size());
}

};