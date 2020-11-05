#include "stdafx.h"

#include "Headers/RenderPassManager.h"

#include "Core/Headers/EngineTaskPool.h"
#include "Core/Headers/Kernel.h"
#include "Core/Headers/PlatformContext.h"
#include "Core/Headers/StringHelper.h"
#include "Core/Resources/Headers/ResourceCache.h"
#include "Core/Time/Headers/ProfileTimer.h"
#include "Editor/Headers/Editor.h"
#include "Managers/Headers/SceneManager.h"
#include "Platform/Video/Buffers/ShaderBuffer/Headers/ShaderBuffer.h"
#include "Platform/Video/Textures/Headers/Texture.h"
#include "Rendering/Camera/Headers/Camera.h"
#include "Rendering/Headers/Renderer.h"
#include "Rendering/PostFX/Headers/PostFX.h"
#include "Rendering/RenderPass/Headers/RenderQueue.h"

#include "ECS/Components/Headers/AnimationComponent.h"
#include "ECS/Components/Headers/BoundsComponent.h"
#include "ECS/Components/Headers/RenderingComponent.h"
#include "ECS/Components/Headers/TransformComponent.h"
#include "Platform/Video/Headers/CommandBufferPool.h"

namespace Divide {

    namespace {
        constexpr U32 g_nodesPerPrepareDrawPartition = 16u;

        struct PerPassData {
            RenderBin::SortedQueues sortedQueues;
            DrawCommandContainer drawCommands;

            std::array<GFXDevice::NodeData, Config::MAX_VISIBLE_NODES> nodeData;
            std::array<GFXDevice::CollisionData, Config::MAX_VISIBLE_NODES> collisionData;
        };

        std::array<PerPassData, to_base(RenderStage::COUNT)> g_passData;
    };

    RenderPassManager::RenderPassManager(Kernel& parent, GFXDevice& context)
        : KernelComponent(parent),
          _context(context),
          _renderQueue(parent),
          _renderPassTimer(&Time::ADD_TIMER("RenderPasses Timer")),
          _buildCommandBufferTimer(&Time::ADD_TIMER("BuildCommandBuffers Timer")),
          _flushCommandBufferTimer(&Time::ADD_TIMER("FlushCommandBuffers Timer")),
          _postFxRenderTimer(&Time::ADD_TIMER("PostFX Timer")),
          _blitToDisplayTimer(&Time::ADD_TIMER("Flush Buffers Timer")),
          _drawCallCount{}
    {
        _buildCommandBufferTimer->addChildTimer(*_blitToDisplayTimer);
        _flushCommandBufferTimer->addChildTimer(*_buildCommandBufferTimer);
        _drawCallCount.fill(0);
    }

    RenderPassManager::~RenderPassManager()
    {
        for (GFX::CommandBuffer*& buf : _renderPassCommandBuffer) {
            GFX::deallocateCommandBuffer(buf);
        }

        if (_postFXCommandBuffer != nullptr) {
            GFX::deallocateCommandBuffer(_postFXCommandBuffer);
        }  
        if (_postRenderBuffer != nullptr) {
            GFX::deallocateCommandBuffer(_postRenderBuffer);
        }
        MemoryManager::DELETE_CONTAINER(_renderPasses);
    }

    void RenderPassManager::postInit() {
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
        _OITCompositionShader = CreateResource<ShaderProgram>(parent().resourceCache(), shaderResDesc);

        PipelineDescriptor pipelineDescriptor;
        pipelineDescriptor._stateHash = _context.get2DStateBlock();
        pipelineDescriptor._shaderProgramHandle = _OITCompositionShader->getGUID();
        _OITCompositionPipeline = _context.newPipeline(pipelineDescriptor);

        _renderPassCommandBuffer.push_back(GFX::allocateCommandBuffer());
        _postFXCommandBuffer = GFX::allocateCommandBuffer();
        _postRenderBuffer = GFX::allocateCommandBuffer();
    }

    void RenderPassManager::render(const RenderParams& params) {
        OPTICK_EVENT();

        if (params._parentTimer != nullptr && !params._parentTimer->hasChildTimer(*_renderPassTimer)) {
            params._parentTimer->addChildTimer(*_renderPassTimer);
            params._parentTimer->addChildTimer(*_postFxRenderTimer);
            params._parentTimer->addChildTimer(*_flushCommandBufferTimer);
        }

        GFXDevice& gfx = _context;
        PlatformContext& context = parent().platformContext();
        SceneManager* sceneManager = parent().sceneManager();

        const SceneRenderState& sceneRenderState = *params._sceneRenderState;
        const Camera* cam = Attorney::SceneManagerRenderPass::playerCamera(sceneManager);
        const SceneStatePerPlayer& playerState = Attorney::SceneManagerRenderPass::playerState(sceneManager);
        gfx.setPreviousViewProjection(playerState.previousViewMatrix(), playerState.previousProjectionMatrix());

        LightPool& activeLightPool = Attorney::SceneManagerRenderPass::lightPool(sceneManager);

        activeLightPool.preRenderAllPasses(cam);

        TaskPool& pool = context.taskPool(TaskPoolType::HIGH_PRIORITY);
        RenderTarget& resolvedScreenTarget = gfx.renderTargetPool().renderTarget(RenderTargetID(RenderTargetUsage::SCREEN));

        const U8 renderPassCount = to_U8(_renderPasses.size());

        Task* postFXTask;
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
                    Start(*_renderTasks[i], TaskPriority::DONT_CARE);
                }
                { //PostFX should be pretty fast
                    GFX::CommandBuffer* buf = _postFXCommandBuffer;

                    Time::ProfileTimer& timer = *_postFxRenderTimer;
                    postFXTask = CreateTask(pool,
                                            nullptr,
                                            [buf, &gfx, &cam, &timer](const Task & parentTask) {
                                                OPTICK_EVENT("PostFX: BuildCommandBuffer");

                                                buf->clear(false);

                                                Time::ScopedTimer time(timer);
                                                gfx.getRenderer().postFX().apply(cam, *buf);
                                                buf->batch();
                                            },
                                            false);
                    Start(*postFXTask, TaskPriority::DONT_CARE);
                }
            }
        }
        {
           GFX::CommandBuffer& buf = *_postRenderBuffer;
           buf.clear(false);

           if (params._editorRunning) {
               GFX::BeginRenderPassCommand beginRenderPassCmd = {};
               beginRenderPassCmd._target = RenderTargetID(RenderTargetUsage::EDITOR);
               beginRenderPassCmd._name = "BLIT_TO_RENDER_TARGET";
               GFX::EnqueueCommand(buf, beginRenderPassCmd);
           }

           GFX::EnqueueCommand(buf, GFX::BeginDebugScopeCommand{ "Flush Display" });

           const auto& screenAtt=  resolvedScreenTarget.getAttachment(RTAttachmentType::Colour, to_U8(GFXDevice::ScreenTargets::ALBEDO));
           const TextureData texData = screenAtt.texture()->data();
           const Rect<I32>& targetViewport = params._targetViewport;
           gfx.drawTextureInViewport(texData, screenAtt.samplerHash(), targetViewport, true, false, buf);
           Attorney::SceneManagerRenderPass::drawCustomUI(sceneManager, targetViewport, buf);
           if_constexpr(Config::Build::ENABLE_EDITOR) {
               context.editor().drawScreenOverlay(cam, targetViewport, buf);
           }
           context.gui().draw(gfx, targetViewport, buf);
           gfx.renderDebugUI(targetViewport, buf);

           GFX::EnqueueCommand(buf, GFX::EndDebugScopeCommand{});
           if (params._editorRunning) {
               GFX::EnqueueCommand(buf, GFX::EndRenderPassCommand{});
           }
        }
        {
            OPTICK_EVENT("RenderPassManager::FlushCommandBuffers");
            Time::ScopedTimer timeCommands(*_flushCommandBufferTimer);

            eastl::fill(eastl::begin(_completedPasses), eastl::end(_completedPasses), false);
            {
                OPTICK_EVENT("FLUSH_PASSES_WHEN_READY");
                U8 idleCount = 0u;
                while (!eastl::all_of(eastl::cbegin(_completedPasses), eastl::cend(_completedPasses), [](const bool v) { return v; })) {

                    // For every render pass
                    bool finished = true;
                    for (U8 i = 0; i < renderPassCount; ++i) {
                        if (_completedPasses[i] || !Finished(*_renderTasks[i])) {
                            continue;
                        }

                        // Grab the list of dependencies
                        const vectorEASTL<U8>& dependencies = _renderPasses[i]->dependencies();

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
                            _drawCallCount[i] = _context.getDrawCallCount();
                            _context.flushCommandBuffer(*_renderPassCommandBuffer[i], false);
                            _drawCallCount[i] = _context.getDrawCallCount() - _drawCallCount[i];

                            _completedPasses[i] = true;
                            //Wait(*whileRendering);

                        } else {
                            finished = false;
                        }
                    }

                    if (!finished) {
                        OPTICK_EVENT("IDLING");
                        if (idleCount++ < 2) {
                            parent().idle(idleCount == 1);
                        }
                    }
                }
            }
        }

        // Flush the postFX stack
        Wait(*postFXTask);
        _context.flushCommandBuffer(*_postFXCommandBuffer, false);

        for (U8 i = 0; i < renderPassCount; ++i) {
            _renderPasses[i]->postRender();
        }

        activeLightPool.postRenderAllPasses(cam);

        Time::ScopedTimer time(*_blitToDisplayTimer);
        gfx.flushCommandBuffer(*_postRenderBuffer);
    }

RenderPass& RenderPassManager::addRenderPass(const Str64& renderPassName,
                                             const U8 orderKey,
                                             const RenderStage renderStage,
                                             const vectorEASTL<U8>& dependencies,
                                             const bool usePerformanceCounters) {
    assert(!renderPassName.empty());

    RenderPass* item = MemoryManager_NEW RenderPass(*this, _context, renderPassName, orderKey, renderStage, dependencies, usePerformanceCounters);
    _renderPasses.push_back(item);

    item->initBufferData();

    //Secondary command buffers. Used in a threaded fashion
    _renderPassCommandBuffer.push_back(GFX::allocateCommandBuffer());

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
            GFX::deallocateCommandBuffer(buf);
            _renderPassCommandBuffer.pop_back();
            break;
        }
    }

    _renderTasks.resize(_renderPasses.size());
    _completedPasses.resize(_renderPasses.size(), false);
}

U32 RenderPassManager::getLastTotalBinSize(const RenderStage renderStage) const {
    return getPassForStage(renderStage).getLastTotalBinSize();
}

RenderPass& RenderPassManager::getPassForStage(const RenderStage renderStage) {
    for (RenderPass* pass : _renderPasses) {
        if (pass->stageFlag() == renderStage) {
            return *pass;
        }
    }

    DIVIDE_UNEXPECTED_CALL();
    return *_renderPasses.front();
}

const RenderPass& RenderPassManager::getPassForStage(const RenderStage renderStage) const {
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
void RenderPassManager::processVisibleNode(const RenderingComponent& rComp,
                                           const RenderStagePass& stagePass,
                                           const bool playAnimations,
                                           const bool shadowMap,
                                           const D64 interpolationFactor,
                                           const bool needsInterp,
                                           GFXDevice::NodeData& dataOut,
                                           GFXDevice::CollisionData& collisionDataOut) const {
    OPTICK_EVENT();

    const SceneGraphNode* node = rComp.getSGN();
    const TransformComponent* const transform = node->get<TransformComponent>();
    assert(transform != nullptr);

    // ... get the node's world matrix properly interpolated
    transform->getPreviousWorldMatrix(dataOut._prevWorldMatrix);
    if (needsInterp) {
        transform->getWorldMatrix(interpolationFactor, dataOut._worldMatrix);
    } else {
        transform->getWorldMatrix(dataOut._worldMatrix);
    }

    dataOut._normalMatrixW.set(dataOut._worldMatrix);
    dataOut._normalMatrixW.setRow(3, 0.0f, 0.0f, 0.0f, 1.0f);
    if (!transform->isUniformScaled()) {
        // Non-uniform scaling requires an inverseTranspose to negate
        // scaling contribution but preserve rotation
        dataOut._normalMatrixW.inverseTranspose();
    }

    // Get the material property matrix (alpha test, texture count, texture operation, etc.)
    bool frameTicked = false;
    if (playAnimations) {
        AnimationComponent* animComp = node->get<AnimationComponent>();
        if (animComp && animComp->playAnimations()) {
            dataOut._normalMatrixW.element(0, 3) = to_F32(animComp->boneCount());
            frameTicked = animComp->frameTicked();
        }
    }

    // Since the normal matrix is 3x3, we can use the extra row and column to store additional data
    const BoundsComponent* const bounds = node->get<BoundsComponent>();
    collisionDataOut._boundingSphere = bounds->getBoundingSphere().asVec4();
    collisionDataOut._boundingBoxHExt = bounds->getBoundingBox().getHalfExtent();

    // Get the colour matrix (base colour, metallic, etc)
    rComp.getMaterialColourMatrix(stagePass, dataOut._colourMatrix);

    RenderingComponent::NodeRenderingProperties properties = {};
    rComp.getRenderingProperties(stagePass, properties);

    dataOut._normalMatrixW.element(1, 3) = to_F32(properties._reflectionIndex);
    dataOut._normalMatrixW.element(2, 3) = to_F32(properties._refractionIndex);
    dataOut._colourMatrix.element(3, 2) = to_F32(properties._lod);
    //set properties.w to negative value to skip occlusion culling for the node
    dataOut._colourMatrix.element(3, 3) = properties._cullFlagValue;

    // Temp: Make the hovered node brighter by setting emissive to something bright
    if (stagePass._stage == RenderStage::DISPLAY && properties._isHovered) {
        FColour4 matColour = dataOut._colourMatrix.getRow(2);
        matColour.rgb = {0.f, 0.25f, 0.f};
        dataOut._colourMatrix.setRow(2, matColour);
    }

    dataOut._prevWorldMatrix.element(0, 3) = to_F32(properties._texOperation);
    dataOut._prevWorldMatrix.element(1, 3) = to_F32(properties._bumpMethod);
    dataOut._prevWorldMatrix.element(2, 3) = frameTicked ? 1.0f : 0.0f;
    dataOut._prevWorldMatrix.element(3, 3) = 1.0f;
}

U32 RenderPassManager::buildBufferData(const RenderStagePass& stagePass,
                                       const SceneRenderState& renderState,
                                       const RenderPassParams& passParams,
                                       bool fullRefresh,
                                       GFX::CommandBuffer& bufferInOut)
{
    OPTICK_EVENT();
    OPTICK_TAG("FULL_REFRESH", fullRefresh);

    PerPassData& passData = g_passData[to_base(stagePass._stage)];
    RefreshNodeDataParams params = {};
    params._drawCommandsInOut = &passData.drawCommands;
    params._bufferInOut = &bufferInOut;
    params._camera = passParams._camera;
    params._stagePass = &stagePass;

    TargetDataBufferParams bufferParams = {};
    bufferParams._targetBuffer = &bufferInOut;
    bufferParams._camera = passParams._camera;

    RenderPass::BufferData bufferData = getPassForStage(stagePass._stage).getBufferData(stagePass);

    if (fullRefresh) {
        const auto IsOccluderBin = [](RenderBinType binType) {
            const auto type = binType._value;
            return type == RenderBinType::RBT_OPAQUE ||
                   type == RenderBinType::RBT_TERRAIN ||
                   type == RenderBinType::RBT_TERRAIN_AUX;
        };

        params._drawCommandsInOut->clear();

        const D64 interpFactor = GFXDevice::FrameInterpolationFactor();
        const bool needsInterp = interpFactor < 0.985;

        bufferParams._writeIndex = bufferData._cmdBuffer->queueWriteIndex();
        bufferParams._dataIndex = 0u;

        U32 nodeCount = 0u;
        const bool playAnimations = renderState.isEnabledOption(SceneRenderState::RenderOptions::PLAY_ANIMATIONS);
        const bool shadowMap = passParams._shadowMappingEnabled;

        bool isOccluder = true;
        bool hasHiZTarget = passParams._targetHIZ._usage != RenderTargetUsage::COUNT;
        for (U8 i = 0; i < to_U8(RenderBinType::RBT_COUNT); ++i) {
            if (hasHiZTarget && isOccluder != IsOccluderBin(RenderBinType::_from_integral(i))) {
                isOccluder = !isOccluder;
                { 
                    //switch between occluders and ocludees here. Unbind RT and copy depth then rebind, maybe?
                }
                hasHiZTarget = false;
            }

            RenderBin::SortedQueue& queue = passData.sortedQueues[i];
            for (RenderingComponent* rComp : queue) {
                
                if (Attorney::RenderingCompRenderPass::onRefreshNodeData(*rComp, params, bufferParams, false)) {
                    GFXDevice::NodeData& data = passData.nodeData[bufferParams._dataIndex];
                    GFXDevice::CollisionData& colData = passData.collisionData[bufferParams._dataIndex];
                    processVisibleNode(*rComp, stagePass, playAnimations, shadowMap, interpFactor, needsInterp, data, colData);
                    ++bufferParams._dataIndex;
                    ++nodeCount;
                }
            }
        }

        *bufferData._lastCommandCount = to_U32(passData.drawCommands.size());
        *bufferData._lastNodeCount = nodeCount;

        {
            OPTICK_EVENT("RenderPassManager::buildBufferData - UpdateBuffers");
            bufferData._nodeData->writeData(
                bufferData._elementOffset,
                *bufferData._lastNodeCount,
                passData.nodeData.data());

            bufferData._colData->writeData(
                bufferData._elementOffset,
                *bufferData._lastNodeCount,
                passData.collisionData.data());

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

        ShaderBufferBinding colBuffer = {};
        colBuffer._binding = ShaderBufferLocation::COLLISION_INFO;
        colBuffer._buffer = bufferData._colData;
        colBuffer._elementRange = { bufferData._elementOffset, *bufferData._lastNodeCount };


        GFX::BindDescriptorSetsCommand descriptorSetCmd = {};
        descriptorSetCmd._set.addShaderBuffer(cmdBuffer);
        descriptorSetCmd._set.addShaderBuffer(dataBuffer);
        descriptorSetCmd._set.addShaderBuffer(colBuffer);
        GFX::EnqueueCommand(bufferInOut, descriptorSetCmd);
    } else {
        for (RenderBin::SortedQueue& queue : passData.sortedQueues) {
            for (RenderingComponent* rComp : queue) {
                Attorney::RenderingCompRenderPass::onRefreshNodeData(*rComp, params, bufferParams, true);
            }
        }
    }

    return  *bufferData._lastNodeCount;
}

U32 RenderPassManager::buildDrawCommands(const RenderPassParams& params, const bool refresh, GFX::CommandBuffer& bufferInOut)
{
    OPTICK_EVENT();

    const RenderStagePass& stagePass = params._stagePass;
    const bool isPrePass = stagePass._passType == RenderPassType::PRE_PASS;

    PerPassData& passData = g_passData[to_base(stagePass._stage)];

    const SceneRenderState& sceneRenderState = parent().sceneManager()->getActiveScene().renderState();

    for (RenderBin::SortedQueue& queue : passData.sortedQueues) {
        queue.resize(0);
        queue.reserve(Config::MAX_VISIBLE_NODES);
    }

    getQueue().getSortedQueues(stagePass._stage, isPrePass, passData.sortedQueues);
    return buildBufferData(stagePass, sceneRenderState, params, refresh, bufferInOut);
}

void RenderPassManager::prepareRenderQueues(const RenderPassParams& params, const VisibleNodeList<>& nodes, bool refreshNodeData, bool transparencyPass, const RenderingOrder renderOrder) {
    OPTICK_EVENT();

    const RenderStagePass& stagePass = params._stagePass;
    const RenderBinType targetBin = transparencyPass ? RenderBinType::RBT_TRANSLUCENT : RenderBinType::RBT_COUNT;
    const SceneRenderState& sceneRenderState = parent().sceneManager()->getActiveScene().renderState();

    const Camera& cam = *params._camera;

    ParallelForDescriptor descriptor = {};
    descriptor._iterCount = to_U32(nodes.size());
    descriptor._partitionSize = g_nodesPerPrepareDrawPartition;
    descriptor._priority = TaskPriority::DONT_CARE;
    descriptor._useCurrentThread = true;
    descriptor._cbk = [&nodes, &cam, &sceneRenderState, &stagePass, refreshNodeData](const Task* parentTask, U32 start, U32 end) {
                        for (U32 i = start; i < end; ++i) {
                            RenderingComponent * rComp = nodes.node(i)._node->get<RenderingComponent>();
                            Attorney::RenderingCompRenderPass::prepareDrawPackage(*rComp, cam, sceneRenderState, stagePass, refreshNodeData);
                        }
                    };

    parallel_for(_parent.platformContext(), descriptor);

    RenderQueue& queue = getQueue();

    queue.refresh(stagePass._stage, targetBin);
    for (size_t i = 0; i < nodes.size(); ++i) {
        const VisibleNode& node = nodes.node(i);
        queue.addNodeToQueue(node._node, stagePass, node._distanceToCameraSq, targetBin);
    }
    // Sort all bins
    queue.sort(stagePass, targetBin, renderOrder);
    
    auto& packageQueue = _renderQueues[to_base(stagePass._stage)];
    packageQueue.resize(0);
    packageQueue.reserve(Config::MAX_VISIBLE_NODES);


    // Draw everything in the depth pass but only draw stuff from the translucent bin in the OIT Pass and everything else in the colour pass
    queue.populateRenderQueues(stagePass,
                               stagePass.isDepthPass() 
                                            ? std::make_pair(RenderBinType::RBT_COUNT, true)
                                            : std::make_pair(RenderBinType::RBT_TRANSLUCENT, transparencyPass),
                               packageQueue);
}

bool RenderPassManager::prePass(const VisibleNodeList<>& nodes, const RenderPassParams& params, const RenderTarget& target, U32& visibleNodeCount, GFX::CommandBuffer& bufferInOut) {
    OPTICK_EVENT();

    assert(params._stagePass._passType == RenderPassType::PRE_PASS);
    const SceneRenderState& activeSceneRenderState = Attorney::SceneManagerRenderPass::renderState(parent().sceneManager());

    // PrePass requires a depth buffer
    const bool doPrePass = params._stagePass._stage != RenderStage::SHADOW &&
                           params._target._usage != RenderTargetUsage::COUNT &&
                           target.getAttachment(RTAttachmentType::Depth, 0).used();

    visibleNodeCount = 0u;
    if (doPrePass) {
        GFX::EnqueueCommand(bufferInOut, GFX::BeginDebugScopeCommand{ " - PrePass" });

        prepareRenderQueues(params, nodes, true, false);
        visibleNodeCount = buildDrawCommands(params, true, bufferInOut);

        const bool layeredRendering = (params._layerParams._layer > 0);

        GFX::BeginRenderPassCommand beginRenderPassCommand = {};
        beginRenderPassCommand._target = params._target;
        beginRenderPassCommand._descriptor = params._targetDescriptorPrePass;
        beginRenderPassCommand._name = "DO_PRE_PASS";
        GFX::EnqueueCommand(bufferInOut, beginRenderPassCommand);

        if (layeredRendering) {
            GFX::BeginRenderSubPassCommand beginRenderSubPassCmd = {};
            beginRenderSubPassCmd._writeLayers.push_back(params._layerParams);
            GFX::EnqueueCommand(bufferInOut, beginRenderSubPassCmd);
        }

        renderQueueToSubPasses(params._stagePass._stage, bufferInOut);

        getQueue().postRender(activeSceneRenderState, params._stagePass, bufferInOut);
        
        if (layeredRendering) {
            GFX::EnqueueCommand(bufferInOut, GFX::EndRenderSubPassCommand{});
        }

        GFX::EnqueueCommand(bufferInOut, GFX::EndRenderPassCommand{});
        
        GFX::EnqueueCommand(bufferInOut, GFX::EndDebugScopeCommand{});
    }

    return doPrePass;
}

bool RenderPassManager::occlusionPass(const VisibleNodeList<>& nodes,
                                      const U32 visibleNodeCount,
                                      const RenderStagePass& stagePass,
                                      const Camera& camera,
                                      const RenderTargetID& sourceDepthBuffer,
                                      const RenderTargetID& targetDepthBuffer,
                                      GFX::CommandBuffer& bufferInOut) const
{
    OPTICK_EVENT();

    //ToDo: Find a way to skip occlusion culling for low number of nodes in view but also keep
    //light culling up and running -Ionut
    ACKNOWLEDGE_UNUSED(visibleNodeCount);
    assert(stagePass._passType == RenderPassType::PRE_PASS);

    if (targetDepthBuffer._usage == RenderTargetUsage::COUNT) {
        return false;
    }

    GFX::EnqueueCommand(bufferInOut, GFX::BeginDebugScopeCommand{ "HiZ Construct & Cull" });

    // Update HiZ Target
    const std::pair<const Texture_ptr&, size_t> HiZTex = _context.constructHIZ(sourceDepthBuffer, targetDepthBuffer, bufferInOut);

    // ToDo: This should not be needed as we unbind the render target before we dispatch the compute task anyway.
    // See if we can remove this -Ionut
    GFX::MemoryBarrierCommand memCmd = {};
    memCmd._barrierMask = to_base(MemoryBarrierType::RENDER_TARGET) | 
                          to_base(MemoryBarrierType::TEXTURE_FETCH) | 
                          to_base(MemoryBarrierType::TEXTURE_BARRIER);
    GFX::EnqueueCommand(bufferInOut, memCmd);

    // Run occlusion culling CS
    const RenderPass::BufferData& bufferData = getBufferData(stagePass);
    GFX::SendPushConstantsCommand HIZPushConstantsCMDInOut = {};
    _context.occlusionCull(stagePass, bufferData, HiZTex.first, HiZTex.second, HIZPushConstantsCMDInOut, bufferInOut);

    GFX::EnqueueCommand(bufferInOut, GFX::BeginDebugScopeCommand{ "Per-node HiZ Cull" });
    for (size_t i = 0; i < nodes.size(); ++i) {
        const VisibleNode& node = nodes.node(i);
        Attorney::SceneGraphNodeRenderPassManager::occlusionCullNode(node._node, stagePass, HiZTex.first, camera, HIZPushConstantsCMDInOut, bufferInOut);
    }
    GFX::EnqueueCommand(bufferInOut, GFX::EndDebugScopeCommand{});

    // Occlusion culling barrier
    memCmd._barrierMask = to_base(MemoryBarrierType::COMMAND_BUFFER) | //For rendering
                          to_base(MemoryBarrierType::SHADER_STORAGE);  //For updating later on

    if (bufferData._cullCounter != nullptr) {
        memCmd._barrierMask |= to_base(MemoryBarrierType::ATOMIC_COUNTER);
        GFX::EnqueueCommand(bufferInOut, memCmd);

        _context.updateCullCount(bufferData, bufferInOut);

        bufferData._cullCounter->incQueue();

        GFX::ClearBufferDataCommand clearAtomicCounter = {};
        clearAtomicCounter._buffer = bufferData._cullCounter;
        clearAtomicCounter._offsetElementCount = 0;
        clearAtomicCounter._elementCount = 1;
        GFX::EnqueueCommand(bufferInOut, clearAtomicCounter);
    } else {
        GFX::EnqueueCommand(bufferInOut, memCmd);
    }

    GFX::EnqueueCommand(bufferInOut, GFX::EndDebugScopeCommand{});

    return true;
}

void RenderPassManager::mainPass(const VisibleNodeList<>& nodes, const RenderPassParams& params, RenderTarget& target, const bool prePassExecuted, const bool hasHiZ, GFX::CommandBuffer& bufferInOut) {
    OPTICK_EVENT();

    const RenderStagePass& stagePass = params._stagePass;
    assert(stagePass._passType == RenderPassType::MAIN_PASS);

    GFX::EnqueueCommand(bufferInOut, GFX::BeginDebugScopeCommand{ " - MainPass" });

    prepareRenderQueues(params, nodes, !prePassExecuted, false);
    buildDrawCommands(params, !prePassExecuted, bufferInOut);

    if (params._target._usage != RenderTargetUsage::COUNT) {
        SceneManager* sceneManager = parent().sceneManager();
        const SceneRenderState& activeSceneRenderState = Attorney::SceneManagerRenderPass::renderState(sceneManager);
        LightPool& activeLightPool = Attorney::SceneManagerRenderPass::lightPool(sceneManager);

        Texture_ptr hizTex = nullptr;
        size_t hizSampler = 0;
        if (hasHiZ) {
            const RenderTarget& hizTarget = _context.renderTargetPool().renderTarget(params._targetHIZ);
            const auto& hizAtt = hizTarget.getAttachment(RTAttachmentType::Depth, 0);
            hizTex = hizAtt.texture();
            hizSampler = hizAtt.samplerHash();
        }

        const RenderTarget& nonMSTarget = _context.renderTargetPool().renderTarget(RenderTargetUsage::SCREEN);

        _context.getRenderer().preRender(stagePass, hizTex, hizSampler, activeLightPool, params._camera, bufferInOut);

        GFX::BindDescriptorSetsCommand descriptorSetCmd = {};
        if (stagePass._stage == RenderStage::DISPLAY) {
            assert(prePassExecuted);
            const auto& normAtt = nonMSTarget.getAttachmentPtr(RTAttachmentType::Colour, to_U8(GFXDevice::ScreenTargets::NORMALS_AND_VELOCITY));
            const auto& gbufferAtt = nonMSTarget.getAttachmentPtr(RTAttachmentType::Colour, to_U8(GFXDevice::ScreenTargets::EXTRA));

            const TextureData normals = normAtt->texture()->data();
            const TextureData extra = gbufferAtt->texture()->data();
            descriptorSetCmd._set._textureData.setTexture(normals, normAtt->samplerHash(), TextureUsage::NORMALMAP);
            descriptorSetCmd._set._textureData.setTexture(extra, gbufferAtt->samplerHash(), TextureUsage::GBUFFER_EXTRA);
        }

        if (hasHiZ) {
            descriptorSetCmd._set._textureData.setTexture(hizTex->data(), hizSampler, TextureUsage::DEPTH);
        } else if (prePassExecuted) {
            if (params._target._usage == RenderTargetUsage::SCREEN_MS) {
                const auto& depthAtt = nonMSTarget.getAttachment(RTAttachmentType::Depth, 0);
                descriptorSetCmd._set._textureData.setTexture(depthAtt.texture()->data(), depthAtt.samplerHash(), TextureUsage::DEPTH);
            } else {
                const auto& depthAtt = target.getAttachment(RTAttachmentType::Depth, 0);
                descriptorSetCmd._set._textureData.setTexture(depthAtt.texture()->data(), depthAtt.samplerHash(), TextureUsage::DEPTH);
            }
        }

        GFX::EnqueueCommand(bufferInOut, descriptorSetCmd);

        const bool layeredRendering = (params._layerParams._layer > 0);

        GFX::BeginRenderPassCommand beginRenderPassCommand = {};
        beginRenderPassCommand._target = params._target;
        beginRenderPassCommand._descriptor = params._targetDescriptorMainPass;
        beginRenderPassCommand._name = "DO_MAIN_PASS";
        GFX::EnqueueCommand(bufferInOut, beginRenderPassCommand);

        if (layeredRendering) {
            GFX::BeginRenderSubPassCommand beginRenderSubPassCmd = {};
            beginRenderSubPassCmd._writeLayers.push_back(params._layerParams);
            GFX::EnqueueCommand(bufferInOut, beginRenderSubPassCmd);
        }

        // We try and render translucent items in the shadow pass and due some alpha-discard tricks
        renderQueueToSubPasses(stagePass._stage, bufferInOut);

        getQueue().postRender(activeSceneRenderState, stagePass, bufferInOut);

        if (stagePass._stage == RenderStage::DISPLAY) {
            /// These should be OIT rendered as well since things like debug nav meshes have translucency
            Attorney::SceneManagerRenderPass::debugDraw(sceneManager, stagePass, params._camera, bufferInOut);
        }

        if (layeredRendering) {
            GFX::EnqueueCommand(bufferInOut, GFX::EndRenderSubPassCommand{});
        }

        GFX::EnqueueCommand(bufferInOut, GFX::EndRenderPassCommand{});
    }

    GFX::EnqueueCommand(bufferInOut, GFX::EndDebugScopeCommand{});
}

void RenderPassManager::woitPass(const VisibleNodeList<>& nodes, const RenderPassParams& params, GFX::CommandBuffer& bufferInOut) {
    const bool isMSAATarget = params._targetOIT._usage == RenderTargetUsage::OIT_MS;

    const RenderStagePass& stagePass = params._stagePass;
    assert(stagePass._passType == RenderPassType::OIT_PASS);

    prepareRenderQueues(params, nodes, false, true);
    buildDrawCommands(params, false, bufferInOut);

    GFX::EnqueueCommand(bufferInOut, GFX::BeginDebugScopeCommand{ " - W-OIT Pass" });

    if (renderQueueSize(stagePass._stage) > 0) {
        GFX::ClearRenderTargetCommand clearRTCmd = {};
        clearRTCmd._target = params._targetOIT;
        if_constexpr(Config::USE_COLOURED_WOIT) {
            // Don't clear our screen target. That would be BAD.
            clearRTCmd._descriptor.clearColour(to_U8(GFXDevice::ScreenTargets::MODULATE), false);
        }
        // Don't clear and don't write to depth buffer
        clearRTCmd._descriptor.clearDepth(false);
        GFX::EnqueueCommand(bufferInOut, clearRTCmd);

        // Step1: Draw translucent items into the accumulation and revealage buffers
        GFX::BeginRenderPassCommand beginRenderPassOitCmd = {};
        beginRenderPassOitCmd._name = "DO_OIT_PASS_1";
        beginRenderPassOitCmd._target = params._targetOIT;
        beginRenderPassOitCmd._descriptor.drawMask().setEnabled(RTAttachmentType::Depth, 0, false);
        GFX::EnqueueCommand(bufferInOut, beginRenderPassOitCmd);

        GFX::SetBlendStateCommand setBlendStateCmd = {};
        {
            RTBlendState& state0 = setBlendStateCmd._blendStates[to_U8(GFXDevice::ScreenTargets::ACCUMULATION)];
            state0._blendProperties._enabled = true;
            state0._blendProperties._blendSrc = BlendProperty::ONE;
            state0._blendProperties._blendDest = BlendProperty::ONE;
            state0._blendProperties._blendOp = BlendOperation::ADD;

            RTBlendState& state1 = setBlendStateCmd._blendStates[to_U8(GFXDevice::ScreenTargets::REVEALAGE)];
            state1._blendProperties._enabled = true;
            state1._blendProperties._blendSrc = BlendProperty::ZERO;
            state1._blendProperties._blendDest = BlendProperty::INV_SRC_COLOR;
            state1._blendProperties._blendOp = BlendOperation::ADD;

            if_constexpr(Config::USE_COLOURED_WOIT) {
                RTBlendState& state2 = setBlendStateCmd._blendStates[to_U8(GFXDevice::ScreenTargets::EXTRA)];
                state2._blendProperties._enabled = true;
                state2._blendProperties._blendSrc = BlendProperty::ONE;
                state2._blendProperties._blendDest = BlendProperty::ONE;
                state2._blendProperties._blendOp = BlendOperation::ADD;
            }
        }
        GFX::EnqueueCommand(bufferInOut, setBlendStateCmd);

        renderQueueToSubPasses(stagePass._stage, bufferInOut/*, quality*/);

        // Reset blend states
        setBlendStateCmd._blendStates = {};
        GFX::EnqueueCommand(bufferInOut, setBlendStateCmd);

        GFX::EndRenderPassCommand endRenderPassCmd = {};
        endRenderPassCmd._setDefaultRTState = isMSAATarget; // We're gonna do a new bind soon enough
        GFX::EnqueueCommand(bufferInOut, endRenderPassCmd);

        if (isMSAATarget) {
            // Blit OIT_MS to OIT
            GFX::BlitRenderTargetCommand blitOITCmd = {};
            blitOITCmd._source = params._targetOIT;
            blitOITCmd._destination = RenderTargetID{ RenderTargetUsage::OIT, params._targetOIT._index };

            for (U8 i = 0; i < 2; ++i) {
                blitOITCmd._blitColours[i].set(i, i, 0u, 0u);
            }

            GFX::EnqueueCommand(bufferInOut, blitOITCmd);
        }

        // Step2: Composition pass
        // Don't clear depth & colours and do not write to the depth buffer
        GFX::EnqueueCommand(bufferInOut, GFX::SetCameraCommand{ Camera::utilityCamera(Camera::UtilityCamera::_2D)->snapshot() });

        const bool layeredRendering = (params._layerParams._layer > 0);
        GFX::BeginRenderPassCommand beginRenderPassCompCmd = {};
        beginRenderPassCompCmd._name = "DO_OIT_PASS_2";
        beginRenderPassCompCmd._target = params._target;
        beginRenderPassCompCmd._descriptor.drawMask().setEnabled(RTAttachmentType::Depth, 0, false);
        GFX::EnqueueCommand(bufferInOut, beginRenderPassCompCmd);

        if (layeredRendering) {
            GFX::BeginRenderSubPassCommand beginRenderSubPassCmd = {};
            beginRenderSubPassCmd._writeLayers.push_back(params._layerParams);
            GFX::EnqueueCommand(bufferInOut, beginRenderSubPassCmd);
        }

        setBlendStateCmd = {};
        {
            RTBlendState& state0 = setBlendStateCmd._blendStates[to_U8(GFXDevice::ScreenTargets::ALBEDO)];
            state0._blendProperties._enabled = true;
            state0._blendProperties._blendOp = BlendOperation::ADD;
            if_constexpr(Config::USE_COLOURED_WOIT) {
                state0._blendProperties._blendSrc = BlendProperty::INV_SRC_ALPHA;
                state0._blendProperties._blendDest = BlendProperty::ONE;
            } else {
                state0._blendProperties._blendSrc = BlendProperty::SRC_ALPHA;
                state0._blendProperties._blendDest = BlendProperty::INV_SRC_ALPHA;
            }
        }
        GFX::EnqueueCommand(bufferInOut, setBlendStateCmd);

        GFX::EnqueueCommand(bufferInOut, GFX::BindPipelineCommand{ _OITCompositionPipeline });

        RenderTarget& oitRT = _context.renderTargetPool().renderTarget(isMSAATarget ? RenderTargetID{ RenderTargetUsage::OIT, params._targetOIT._index } : params._targetOIT);
        const auto& accumAtt = oitRT.getAttachment(RTAttachmentType::Colour, to_U8(GFXDevice::ScreenTargets::ACCUMULATION));
        const auto& revAtt = oitRT.getAttachment(RTAttachmentType::Colour, to_U8(GFXDevice::ScreenTargets::REVEALAGE));

        const TextureData accum = accumAtt.texture()->data();
        const TextureData revealage = revAtt.texture()->data();

        GFX::BindDescriptorSetsCommand descriptorSetCmd = {};
        descriptorSetCmd._set._textureData.setTexture(accum, accumAtt.samplerHash(),to_base(TextureUsage::UNIT0));
        descriptorSetCmd._set._textureData.setTexture(revealage, revAtt.samplerHash(), to_base(TextureUsage::UNIT1));
        GFX::EnqueueCommand(bufferInOut, descriptorSetCmd);

        GenericDrawCommand drawCommand = {};
        drawCommand._primitiveType = PrimitiveType::TRIANGLES;

        GFX::EnqueueCommand(bufferInOut, GFX::DrawCommand{ drawCommand });

        if (layeredRendering) {
            GFX::EnqueueCommand(bufferInOut, GFX::EndRenderSubPassCommand{});
        }

        // Reset blend states
        setBlendStateCmd._blendStates = {};
        GFX::EnqueueCommand(bufferInOut, setBlendStateCmd);

        GFX::EnqueueCommand(bufferInOut, GFX::EndRenderPassCommand{});
    }

    GFX::EnqueueCommand(bufferInOut, GFX::EndDebugScopeCommand{});
}

void RenderPassManager::transparencyPass(const VisibleNodeList<>& nodes, const RenderPassParams& params, GFX::CommandBuffer& bufferInOut) {
    OPTICK_EVENT();
    if (params._stagePass._stage == RenderStage::SHADOW) {
        return;
    }

    if (params._stagePass._passType == RenderPassType::OIT_PASS) {
        woitPass(nodes, params, bufferInOut);
    } else {
        assert(params._stagePass._passType == RenderPassType::MAIN_PASS);

        //Grab all transparent geometry
        prepareRenderQueues(params, nodes, false, true, RenderingOrder::BACK_TO_FRONT);
        buildDrawCommands(params, false, bufferInOut);

        GFX::EnqueueCommand(bufferInOut, GFX::BeginDebugScopeCommand{ " - Transparency Pass" });

        if (renderQueueSize(params._stagePass._stage) > 0) {
            const bool layeredRendering = (params._layerParams._layer > 0);

            GFX::BeginRenderPassCommand beginRenderPassTransparentCmd = {};
            beginRenderPassTransparentCmd._name = "DO_TRANSPARENCY_PASS";
            beginRenderPassTransparentCmd._target = params._target;
            beginRenderPassTransparentCmd._descriptor.drawMask().setEnabled(RTAttachmentType::Depth, 0, false);
            GFX::EnqueueCommand(bufferInOut, beginRenderPassTransparentCmd);

            if (layeredRendering) {
                GFX::BeginRenderSubPassCommand beginRenderSubPassCmd = {};
                beginRenderSubPassCmd._writeLayers.push_back(params._layerParams);
                GFX::EnqueueCommand(bufferInOut, beginRenderSubPassCmd);
            }

            GFX::SetBlendStateCommand setBlendStateCmd = {};
            RTBlendState& state0 = setBlendStateCmd._blendStates[to_U8(GFXDevice::ScreenTargets::ALBEDO)];
            state0._blendProperties._enabled = true;
            state0._blendProperties._blendSrc = BlendProperty::SRC_ALPHA;
            state0._blendProperties._blendDest = BlendProperty::INV_SRC_ALPHA;
            state0._blendProperties._blendOp = BlendOperation::ADD;

            renderQueueToSubPasses(params._stagePass._stage, bufferInOut/*, quality*/);

            // Reset blend states
            setBlendStateCmd._blendStates = {};
            GFX::EnqueueCommand(bufferInOut, setBlendStateCmd);

            if (layeredRendering) {
                GFX::EnqueueCommand(bufferInOut, GFX::EndRenderSubPassCommand{});
            }

            GFX::EnqueueCommand(bufferInOut, GFX::EndRenderPassCommand{});
        }

        GFX::EnqueueCommand(bufferInOut, GFX::EndDebugScopeCommand{});
    }
}

void RenderPassManager::doCustomPass(RenderPassParams params, GFX::CommandBuffer& bufferInOut) {
    OPTICK_EVENT();

    const RenderStage stage = params._stagePass._stage;

    params._camera->updateLookAt();
    const CameraSnapshot& camSnapshot = params._camera->snapshot();

     GFX::EnqueueCommand(bufferInOut, GFX::BeginDebugScopeCommand{
         Util::StringFormat("Custom pass ( %s - %s )", TypeUtil::RenderStageToString(stage), params._passName.empty() ? "N/A" : params._passName.c_str()).c_str()
     });

    const bool layeredRendering = (params._layerParams._layer > 0);
    if (!layeredRendering) {
        Attorney::SceneManagerRenderPass::prepareLightData(parent().sceneManager(), stage, camSnapshot._eye, camSnapshot._viewMatrix);
    }

    // Tell the Rendering API to draw from our desired PoV
    GFX::EnqueueCommand(bufferInOut, GFX::SetCameraCommand{ camSnapshot });
    GFX::EnqueueCommand(bufferInOut, GFX::SetClipPlanesCommand{ params._clippingPlanes });

    RenderTarget& target = _context.renderTargetPool().renderTarget(params._target);

    // Cull the scene and grab the visible nodes
    I64 ignoreGUID = params._sourceNode == nullptr ? -1 : params._sourceNode->getGUID();
    const VisibleNodeList<>& visibleNodes = Attorney::SceneManagerRenderPass::cullScene(parent().sceneManager(), stage, *params._camera, params._minLoD, params._minExtents, &ignoreGUID, 1);

    if (params._feedBackContainer != nullptr) {
        auto& container = params._feedBackContainer->_visibleNodes;
        container.resize(visibleNodes.size());
        std::memcpy(container.data(), visibleNodes.data(), visibleNodes.size() * sizeof(VisibleNode));
    }

    U32 visibleNodeCount = 0u;
#   pragma region PRE_PASS
        params._stagePass._passType = RenderPassType::PRE_PASS;
        const bool prePassExecuted = prePass(visibleNodes, params, target, visibleNodeCount, bufferInOut);
#   pragma endregion

    // If we rendered to the multisampled screen target, we can now copy the g-buffer data to our regular buffer as we are done with it at this point
    RenderTargetID sourceID = params._target;
    if (params._target._usage == RenderTargetUsage::SCREEN_MS) {
        const RenderTargetID targetID = { RenderTargetUsage::SCREEN, params._target._index };

        GFX::BlitRenderTargetCommand blitScreenDepthCmd = {};
        blitScreenDepthCmd._source = sourceID;
        blitScreenDepthCmd._destination = targetID;
        
        for (U8 i = 1; i < to_base(GFXDevice::ScreenTargets::COUNT); ++i) {
            blitScreenDepthCmd._blitColours[i - 1].set(i, i, 0u, 0u);
        }

        DepthBlitEntry entry;
        entry._inputLayer = entry._outputLayer = 0u;
        blitScreenDepthCmd._blitDepth = entry;
        GFX::EnqueueCommand(bufferInOut, blitScreenDepthCmd);

        sourceID = targetID;
    }

#   pragma region HI_Z
        const bool hasHiZ = prePassExecuted && 
                            occlusionPass(visibleNodes, visibleNodeCount, params._stagePass, *params._camera, sourceID, params._targetHIZ, bufferInOut);
#   pragma endregion

    if (stage == RenderStage::DISPLAY) {
        //ToDo: Might be worth having pre-pass operations per render stage, but currently, only the main pass needs SSAO, bloom and so forth
        _context.getRenderer().postFX().prepare(params._camera, bufferInOut);
    }

#   pragma region MAIN_PASS
        params._stagePass._passType = RenderPassType::MAIN_PASS;
        mainPass(visibleNodes, params, target, prePassExecuted, hasHiZ, bufferInOut);
#   pragma endregion

#   pragma region TRANSPARENCY_PASS
    if (params._targetOIT._usage != RenderTargetUsage::COUNT) {
        params._stagePass._passType = RenderPassType::OIT_PASS;
    } else {
        // Use forward pass shaders
        params._stagePass._passType = RenderPassType::MAIN_PASS;
    }
    transparencyPass(visibleNodes, params, bufferInOut);
#   pragma endregion

    // If we rendered to the multisampled screen target, we can now copy the colour to our regular buffer as we are done with it at this point
    if (params._target._usage == RenderTargetUsage::SCREEN_MS) {
        sourceID = params._target;
        const RenderTargetID targetID = { RenderTargetUsage::SCREEN, params._target._index };

        GFX::BlitRenderTargetCommand blitScreenDepthCmd = {};
        blitScreenDepthCmd._source = sourceID;
        blitScreenDepthCmd._destination = targetID;

        blitScreenDepthCmd._blitColours[0].set(to_U16(GFXDevice::ScreenTargets::ALBEDO), to_U16(GFXDevice::ScreenTargets::ALBEDO), 0u, 0u);

        GFX::EnqueueCommand(bufferInOut, blitScreenDepthCmd);
    }

    GFX::EnqueueCommand(bufferInOut, GFX::EndDebugScopeCommand{});
}


// TEMP
U32 RenderPassManager::renderQueueSize(const RenderStage stage, const RenderPackage::MinQuality qualityRequirement) const {
    const auto& queue = _renderQueues[to_base(stage)];
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

void RenderPassManager::renderQueueToSubPasses(const RenderStage stage, GFX::CommandBuffer& commandsInOut, const RenderPackage::MinQuality qualityRequirement) const {
    OPTICK_EVENT();

    const vectorEASTLFast<RenderPackage*>& queue = _renderQueues[to_base(stage)];

    eastl::fixed_vector<GFX::CommandBuffer*, Config::MAX_VISIBLE_NODES> buffers = {};

    if (qualityRequirement == RenderPackage::MinQuality::COUNT) {
        for (RenderPackage* item : queue) {
            buffers.push_back(Attorney::RenderPackageRenderPassManager::getCommandBuffer(item));
        }
    } else {
        for (RenderPackage* item : queue) {
            if (item->qualityRequirement() == qualityRequirement) {
                buffers.push_back(Attorney::RenderPackageRenderPassManager::getCommandBuffer(item));
            }
        }
    }

    commandsInOut.add(buffers.data(), buffers.size());
}

};