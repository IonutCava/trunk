#include "stdafx.h"

#include "Headers/RenderPassManager.h"

#include "Core/Headers/Kernel.h"
#include "Core/Headers/TaskPool.h"
#include "Core/Headers/StringHelper.h"
#include "Core/Headers/Configuration.h"
#include "Core/Headers/PlatformContext.h"
#include "Core/Time/Headers/ProfileTimer.h"
#include "Managers/Headers/SceneManager.h"
#include "Rendering/Camera/Headers/Camera.h"
#include "Rendering/PostFX/Headers/PostFX.h"
#include "Rendering/RenderPass/Headers/RenderQueue.h"
#include "Platform/Video/Textures/Headers/Texture.h"
#include "Platform/Video/Buffers/ShaderBuffer/Headers/ShaderBuffer.h"

#include "ECS/Components/Headers/BoundsComponent.h"
#include "ECS/Components/Headers/RenderingComponent.h"
#include "ECS/Components/Headers/TransformComponent.h"
#include "ECS/Components/Headers/AnimationComponent.h"


#ifndef USE_COLOUR_WOIT
//#define USE_COLOUR_WOIT
#endif

namespace Divide {

    namespace {
        typedef std::set<U32> SetType;
        thread_local SetType g_usedIndices;
        thread_local U32 g_freeCounter = 0;
        thread_local RenderQueue::SortedQueues g_sortedQueues;
        thread_local std::array<GFXDevice::NodeData, Config::MAX_VISIBLE_NODES> g_nodeData;
        thread_local vectorEASTL<IndirectDrawCommand> g_drawCommands;
    };

    std::atomic_uint RenderPassManager::g_NodeDataIndex = 0;
    U32 RenderPassManager::getUniqueNodeDataIndex() {
        return g_NodeDataIndex.fetch_add(1);
    }

    RenderPassManager::RenderPassManager(Kernel& parent, GFXDevice& context)
        : KernelComponent(parent),
        _renderQueue(parent),
        _context(context),
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
        shaderResDesc.setPropertyDescriptor(shaderDescriptor);
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
    }

    void RenderPassManager::postInit() {
        WAIT_FOR_CONDITION(_OITCompositionShader->getState() == ResourceState::RES_LOADED);
    }

    void RenderPassManager::render(SceneRenderState& sceneRenderState, Time::ProfileTimer* parentTimer) {
        if (parentTimer != nullptr && !parentTimer->hasChildTimer(*_renderPassTimer)) {
            parentTimer->addChildTimer(*_renderPassTimer);
            parentTimer->addChildTimer(*_postFxRenderTimer);
            parentTimer->addChildTimer(*_flushCommandBufferTimer);
        }

        const Camera& cam = Attorney::SceneManagerRenderPass::playerCamera(parent().sceneManager());

        Attorney::SceneManagerRenderPass::preRenderAllPasses(parent().sceneManager(), cam);

        TaskPool& pool = parent().platformContext().taskPool(TaskPoolType::HIGH_PRIORITY);

        U8 renderPassCount = to_U8(_renderPasses.size());

        vector<Task*> tasks(renderPassCount);
        Task* postFXTask = nullptr;

        {
            Time::ScopedTimer timeCommands(*_buildCommandBufferTimer);
            {
                Time::ScopedTimer timeAll(*_renderPassTimer);

                for (I8 i = 0; i < renderPassCount; ++i)
                { //All of our render passes should run in parallel

                    RenderPass* pass = _renderPasses[i].get();

                    GFX::CommandBuffer* buf = _renderPassCommandBuffer[i];
                    assert(buf->empty());

                    tasks[i] = CreateTask(pool,
                                          nullptr,
                                          [pass, buf, &sceneRenderState](const Task & parentTask) {
                                              pass->render(parentTask, sceneRenderState, *buf);
                                              buf->batch();
                                          },
                                          "Render pass task");
                    Start(*tasks[i]);
                }
                { //PostFX should be pretty fast
                    GFX::CommandBuffer* buf = _postFXCommandBuffer;
                    assert(buf->empty());

                    Time::ProfileTimer& timer = *_postFxRenderTimer;
                    GFXDevice& gfx = parent().platformContext().gfx();
                    PostFX& postFX = gfx.postFX();

                    postFXTask = CreateTask(pool,
                                            nullptr,
                                            [buf, &gfx, &postFX, &cam, &timer](const Task & parentTask) {
                                                Time::ScopedTimer time(timer);
                                                postFX.apply(cam, *buf);

                                                GFX::CopyTextureCommand copyCmd = {};
                                                copyCmd._source = gfx.renderTargetPool().renderTarget(RenderTargetID(RenderTargetUsage::HI_Z)).getAttachment(RTAttachmentType::Depth, 0).texture();
                                                copyCmd._destination = gfx.getPrevDepthBuffer();
                                                GFX::EnqueueCommand(*buf, copyCmd);

                                                buf->batch();
                                            },
                                            "PostFX pass task");
                    Start(*postFXTask);
                }
            }
        }
        {
            Time::ScopedTimer timeCommands(*_flushCommandBufferTimer);

            vectorEASTL<bool> completedPasses(renderPassCount, false);

            bool slowIdle = false;
            while (!eastl::all_of(eastl::cbegin(completedPasses), eastl::cend(completedPasses), [](bool v) { return v; })) {
                // For every render pass
                bool finished = true;
                for (U8 i = 0; i < renderPassCount; ++i) {
                    if (completedPasses[i] || !Finished(*tasks[i])) {
                        continue;
                    }

                    // Grab the list of dependencies
                    const vector<U8>& dependencies = _renderPasses[i]->dependencies();

                    bool dependenciesRunning = false;
                    // For every dependency in the list
                    for (U8 dep : dependencies) {
                        if (dependenciesRunning) {
                            break;
                        }

                        // Try and see if it's running
                        for (U8 j = 0; j < renderPassCount; ++j) {
                            // If it is running, we can't render yet
                            if (j != i && _renderPasses[j]->sortKey() == dep && !completedPasses[j]) {
                                dependenciesRunning = true;
                                break;
                            }
                        }
                    }

                    if (!dependenciesRunning) {
                        // No running dependency so we can flush the command buffer and add the pass to the skip list
                        _context.flushCommandBuffer(*_renderPassCommandBuffer[i]);
                        _renderPassCommandBuffer[i]->clear(false);
                        completedPasses[i] = true;
                    } else {
                        finished = false;
                    }
                }

                if (!finished) {
                    parent().idle(!slowIdle);
                    std::this_thread::yield();
                    slowIdle = !slowIdle;
                }
            }
        }

        // Flush the postFX stack
        Wait(*postFXTask);
        _context.flushCommandBuffer(*_postFXCommandBuffer);
        _postFXCommandBuffer->clear(false);

        for (U8 i = 0; i < renderPassCount; ++i) {
            _renderPasses[i]->postRender();
        }

        Attorney::SceneManagerRenderPass::postRenderAllPasses(parent().sceneManager(), cam);
    }

RenderPass& RenderPassManager::addRenderPass(const stringImpl& renderPassName,
                                             U8 orderKey,
                                             RenderStage renderStage,
                                             vector<U8> dependencies,
                                             bool usePerformanceCounters) {
    assert(!renderPassName.empty());

    _renderPasses.push_back(std::make_shared<RenderPass>(*this, _context, renderPassName, orderKey, renderStage, dependencies, usePerformanceCounters));
    _renderPasses.back()->initBufferData();

    //Secondary command buffers. Used in a threaded fashion
    _renderPassCommandBuffer.push_back(GFX::allocateCommandBuffer(true));

    std::shared_ptr<RenderPass>& item = _renderPasses.back();

    eastl::sort(eastl::begin(_renderPasses),
                eastl::end(_renderPasses),
                [](const std::shared_ptr<RenderPass>& a, const std::shared_ptr<RenderPass>& b) -> bool {
                      return a->sortKey() < b->sortKey();
                });

    return *item;
}

void RenderPassManager::removeRenderPass(const stringImpl& name) {
    for (vectorEASTL<std::shared_ptr<RenderPass>>::iterator it = eastl::begin(_renderPasses);
         it != eastl::end(_renderPasses); ++it) {
         if ((*it)->name().compare(name) == 0) {
            _renderPasses.erase(it);
            // Remove one command buffer
            GFX::CommandBuffer* buf = _renderPassCommandBuffer.back();
            GFX::deallocateCommandBuffer(buf, true);
            _renderPassCommandBuffer.pop_back();
            break;
        }
    }
}

U16 RenderPassManager::getLastTotalBinSize(RenderStage renderStage) const {
    return getPassForStage(renderStage).getLastTotalBinSize();
}

RenderPass& RenderPassManager::getPassForStage(RenderStage renderStage) {
    for (std::shared_ptr<RenderPass>& pass : _renderPasses) {
        if (pass->stageFlag() == renderStage) {
            return *pass;
        }
    }

    DIVIDE_UNEXPECTED_CALL();
    return *_renderPasses.front();
}

const RenderPass& RenderPassManager::getPassForStage(RenderStage renderStage) const {
    for (const std::shared_ptr<RenderPass>& pass : _renderPasses) {
        if (pass->stageFlag() == renderStage) {
            return *pass;
        }
    }

    DIVIDE_UNEXPECTED_CALL();
    return *_renderPasses.front();
}

RenderPass::BufferData RenderPassManager::getBufferData(RenderStagePass stagePass) const {
    return getPassForStage(stagePass._stage).getBufferData(stagePass._passType, stagePass._passIndex);
}

/// Prepare the list of visible nodes for rendering
GFXDevice::NodeData RenderPassManager::processVisibleNode(SceneGraphNode* node, RenderStagePass stagePass, bool playAnimations, const mat4<F32>& viewMatrix) const {
    GFXDevice::NodeData dataOut;

    // Extract transform data (if available)
    // (Nodes without transforms just use identity matrices)
    TransformComponent* const transform = node->get<TransformComponent>();
    if (transform) {
        // ... get the node's world matrix properly interpolated
        transform->getWorldMatrix(_context.getFrameInterpolationFactor(), dataOut._worldMatrix);

        dataOut._normalMatrixW.set(dataOut._worldMatrix);
        if (!transform->isUniformScaled()) {
            // Non-uniform scaling requires an inverseTranspose to negate
            // scaling contribution but preserve rotation
            dataOut._normalMatrixW.setRow(3, 0.0f, 0.0f, 0.0f, 1.0f);
            dataOut._normalMatrixW.inverseTranspose();
        }
    }

    // Get the material property matrix (alpha test, texture count, texture operation, etc.)
    AnimationComponent* animComp = nullptr;
    if (playAnimations) {
        animComp = node->get<AnimationComponent>();
        if (animComp && animComp->playAnimations()) {
            dataOut._normalMatrixW.element(0, 3) = to_F32(animComp->boneCount());
        }
    }

    RenderingComponent* const renderable = node->get<RenderingComponent>();

    vec4<F32> properties = {};
    renderable->getRenderingProperties(stagePass,
                                       properties,
                                       dataOut._normalMatrixW.element(1, 3),
                                       dataOut._normalMatrixW.element(2, 3));

    // Since the normal matrix is 3x3, we can use the extra row and column to store additional data
    BoundsComponent* const bounds = node->get<BoundsComponent>();
    dataOut._normalMatrixW.setRow(3, bounds->getBoundingSphere().asVec4());

    // Get the colour matrix (diffuse, specular, etc.)
    renderable->getMaterialColourMatrix(dataOut._colourMatrix);

    vec4<F32> dataRow = dataOut._colourMatrix.getRow(3);
    dataRow.z = properties.z; // lod
    //set properties.w to negative value to skip occlusion culling for the node
    dataRow.w = properties.w;
    dataOut._colourMatrix.setRow(3, dataRow);

    // Temp: Make the hovered/selected node brighter. 
    if (properties.x > 0.5f || properties.x < -0.5f) {
        FColour4 matColour = dataOut._colourMatrix.getRow(0);
        if (properties.x < -0.5f) {
            matColour.rgb(matColour.rgb() * 3);
        } else {
            matColour.rgb(matColour.rgb() * 2);
        }
        dataOut._colourMatrix.setRow(0, matColour);
    }

    return dataOut;
}

void RenderPassManager::buildBufferData(RenderStagePass stagePass,
                                        const SceneRenderState& renderState,
                                        const Camera& camera,
                                        const RenderQueue::SortedQueues& sortedQueues,
                                        GFX::CommandBuffer& bufferInOut)
{
    bool playAnimations = renderState.isEnabledOption(SceneRenderState::RenderOptions::PLAY_ANIMATIONS);
    g_usedIndices.clear();
    g_freeCounter = 0;
    g_drawCommands.resize(0);
    g_drawCommands.reserve(Config::MAX_VISIBLE_NODES);

    for (const vectorEASTLFast<SceneGraphNode*>& queue : sortedQueues) {
        for (SceneGraphNode* node : queue) {
            RenderingComponent& renderable = *node->get<RenderingComponent>();
            U32 dataIdxOut = 0;
            if (renderable.getDataIndex(dataIdxOut)) {
                g_usedIndices.insert(dataIdxOut);
            }
        }
    }

    bool skip = false;
    U32 totalNodes = 0;
    for (const vectorEASTLFast<SceneGraphNode*>& queue : sortedQueues) {
        if (skip) {
            break;
        }

        for (SceneGraphNode* node : queue) {
            RenderingComponent& renderable = *node->get<RenderingComponent>();

            RefreshNodeDataParams params(g_drawCommands, bufferInOut);
            params._camera = &camera;
            params._stagePass = stagePass;
            if (totalNodes == Config::MAX_VISIBLE_NODES) {
                skip = true;
                break;
            }

            if (!renderable.getDataIndex(params._dataIdx)) {
                SetType::iterator iter = g_usedIndices.lower_bound(g_freeCounter);
                while (iter != std::end(g_usedIndices) && *iter == g_freeCounter) {
                    ++iter;
                    ++g_freeCounter;
                }
                params._dataIdx = g_freeCounter;
            }

            if (Attorney::RenderingCompRenderPass::onRefreshNodeData(renderable, params)) {
                GFXDevice::NodeData data = processVisibleNode(node, stagePass, playAnimations, camera.getViewMatrix());
                g_nodeData[params._dataIdx] = data;
                g_usedIndices.insert(params._dataIdx);
                ++g_freeCounter;
                ++totalNodes;
            }
        }
    }

    U32 nodeCount = std::max(totalNodes, to_U32(*std::max_element(std::cbegin(g_usedIndices), std::cend(g_usedIndices)) + 1));
    U32 cmdCount = to_U32(g_drawCommands.size());

    RenderPass::BufferData bufferData = getBufferData(stagePass);
    *bufferData._lastCommandCount = cmdCount;

    bufferData._cmdBuffer->writeData(
        0,
        cmdCount,
        (bufferPtr)g_drawCommands.data());

    bufferData._renderData->writeData(
        bufferData._renderDataElementOffset,
        nodeCount,
        (bufferPtr)g_nodeData.data());

    ShaderBufferBinding cmdBuffer = {};
    cmdBuffer._binding = ShaderBufferLocation::CMD_BUFFER;
    cmdBuffer._buffer = bufferData._cmdBuffer;
    cmdBuffer._elementRange = { 0u, cmdCount };

    ShaderBufferBinding dataBuffer = {};
    dataBuffer._binding = ShaderBufferLocation::NODE_INFO;
    dataBuffer._buffer = bufferData._renderData;
    dataBuffer._elementRange = { bufferData._renderDataElementOffset, nodeCount };

    GFX::BindDescriptorSetsCommand descriptorSetCmd = {};
    descriptorSetCmd._set.addShaderBuffer(cmdBuffer);
    descriptorSetCmd._set.addShaderBuffer(dataBuffer);
    GFX::EnqueueCommand(bufferInOut, descriptorSetCmd);
}

void RenderPassManager::buildDrawCommands(RenderStagePass stagePass, const PassParams& params, bool refresh, GFX::CommandBuffer& bufferInOut)
{
    const SceneRenderState& sceneRenderState = parent().sceneManager().getActiveScene().renderState();

    U16 queueSize = 0;
    for (auto& queue : g_sortedQueues) {
        queue.resize(0);
        queue.reserve(Config::MAX_VISIBLE_NODES);
    }

    getQueue().getSortedQueues(stagePass, g_sortedQueues, queueSize);
    for (const vectorEASTLFast<SceneGraphNode*>& queue : g_sortedQueues) {
        for (SceneGraphNode* node : queue) {
            if (params._sourceNode != nullptr && *params._sourceNode == *node) {
                continue;
            }

            Attorney::RenderingCompRenderPass::prepareDrawPackage(*node->get<RenderingComponent>(), *params._camera, sceneRenderState, stagePass, refresh);
        }
    }

    if (refresh) {
        buildBufferData(stagePass, sceneRenderState, *params._camera, g_sortedQueues, bufferInOut);
    }
}

void RenderPassManager::prepareRenderQueues(RenderStagePass stagePass, const PassParams& params, const VisibleNodeList& nodes, bool refreshNodeData, GFX::CommandBuffer& bufferInOut) {
    RenderStage stage = stagePass._stage;

    RenderQueue& queue = getQueue();
    queue.refresh(stage);
    for (const VisibleNode& node : nodes) {
        queue.addNodeToQueue(*node._node, stagePass, node._distanceToCameraSq);
    }
    // Sort all bins
    queue.sort(stagePass);
    
    vectorEASTLFast<RenderPackage*>& packageQueue = _renderQueues[to_base(stage)];
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

    buildDrawCommands(stagePass, params, refreshNodeData, bufferInOut);
}

bool RenderPassManager::prePass(const VisibleNodeList& nodes, const PassParams& params, const RenderTarget& target, GFX::CommandBuffer& bufferInOut) {
    // PrePass requires a depth buffer
    bool doPrePass = params._stage != RenderStage::SHADOW &&
                     params._target._usage != RenderTargetUsage::COUNT &&
                     target.getAttachment(RTAttachmentType::Depth, 0).used();

    // We need to add a barrier here for various buffer updates: grass/tree culling, draw command buffer updates, etc
    GFX::MemoryBarrierCommand memCmd;
    memCmd._barrierMask = to_base(MemoryBarrierType::SHADER_BUFFER);
    GFX::EnqueueCommand(bufferInOut, memCmd);

    if (doPrePass) {
        const bool hasNormalsTarget = target.hasAttachment(RTAttachmentType::Colour, to_base(GFXDevice::ScreenTargets::NORMALS_AND_VELOCITY));
        const bool hasLightingTarget = target.hasAttachment(RTAttachmentType::Colour, to_base(GFXDevice::ScreenTargets::EXTRA));

        GFX::BeginDebugScopeCommand beginDebugScopeCmd;
        beginDebugScopeCmd._scopeID = 0;
        beginDebugScopeCmd._scopeName = " - PrePass";
        GFX::EnqueueCommand(bufferInOut, beginDebugScopeCmd);

        RenderStagePass stagePass(params._stage, RenderPassType::PRE_PASS, params._passVariant, params._passIndex);
        prepareRenderQueues(stagePass, params, nodes, true, bufferInOut);

        RTDrawDescriptor normalsAndDepthPolicy = {};
        normalsAndDepthPolicy.enableState(RTDrawDescriptor::State::CLEAR_COLOUR_BUFFERS);
        normalsAndDepthPolicy.clearColour(to_U8(GFXDevice::ScreenTargets::ALBEDO), false);
        normalsAndDepthPolicy.drawMask().disableAll();
        normalsAndDepthPolicy.drawMask().setEnabled(RTAttachmentType::Depth, 0, true);

        if (hasLightingTarget) {
            normalsAndDepthPolicy.clearColour(to_U8(GFXDevice::ScreenTargets::EXTRA), true);
            normalsAndDepthPolicy.drawMask().setEnabled(RTAttachmentType::Colour, to_base(GFXDevice::ScreenTargets::EXTRA), true);
        }

        if (hasNormalsTarget) {
            normalsAndDepthPolicy.clearColour(to_U8(GFXDevice::ScreenTargets::NORMALS_AND_VELOCITY), true);
            normalsAndDepthPolicy.drawMask().setEnabled(RTAttachmentType::Colour, to_base(GFXDevice::ScreenTargets::NORMALS_AND_VELOCITY), true);
        }

        if (params._bindTargets) {
            GFX::BeginRenderPassCommand beginRenderPassCommand;
            beginRenderPassCommand._target = params._target;
            if (hasNormalsTarget) {
                beginRenderPassCommand._descriptor = normalsAndDepthPolicy;
            } else {
                beginRenderPassCommand._descriptor = RenderTarget::defaultPolicyDepthOnly();
            }
            beginRenderPassCommand._name = "DO_PRE_PASS";
            GFX::EnqueueCommand(bufferInOut, beginRenderPassCommand);
        }

        renderQueueToSubPasses(stagePass, bufferInOut);

        Attorney::SceneManagerRenderPass::postRender(parent().sceneManager(), stagePass, *params._camera, bufferInOut);

        if (params._bindTargets) {
            GFX::EndRenderPassCommand endRenderPassCommand;
            GFX::EnqueueCommand(bufferInOut, endRenderPassCommand);
        }

        GFX::EndDebugScopeCommand endDebugScopeCmd;
        GFX::EnqueueCommand(bufferInOut, endDebugScopeCmd);
    }

    return doPrePass;
}

void RenderPassManager::occlusionPass(const VisibleNodeList& nodes, const PassParams& params, const RenderTarget& target, GFX::CommandBuffer& bufferInOut, bool prePassExecuted) {
    ACKNOWLEDGE_UNUSED(nodes);

    if (params._targetHIZ._usage != RenderTargetUsage::COUNT) {

        // Update HiZ Target
        const Texture_ptr& HiZTex = _context.constructHIZ(params._target, params._targetHIZ, bufferInOut);

        // Run occlusion culling CS
        const RenderPass::BufferData& bufferData = getBufferData(RenderStagePass(params._stage, RenderPassType::PRE_PASS, params._passVariant, params._passIndex));
        _context.occlusionCull(bufferData, HiZTex, *params._camera, bufferInOut);

        // Occlusion culling barrier
        GFX::MemoryBarrierCommand memCmd;
        memCmd._barrierMask = to_base(MemoryBarrierType::SHADER_BUFFER);

        if (bufferData._cullCounter != nullptr) {
            memCmd._barrierMask |= to_base(MemoryBarrierType::COUNTER);
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

        // Bind the HiZ target as our depth texture
        GFX::BindDescriptorSetsCommand bindDescriptorSets = {};
        bindDescriptorSets._set._textureData.setTexture(HiZTex->getData(), to_U8(ShaderProgram::TextureUsage::DEPTH));
        GFX::EnqueueCommand(bufferInOut, bindDescriptorSets);
    } else {
        GFX::ResolveRenderTargetCommand resolveCmd = { };
        resolveCmd._source = params._target;
        resolveCmd._resolveDepth = true;
        GFX::EnqueueCommand(bufferInOut, resolveCmd);

        // Bind the regular depth buffer as our depth texture
        const RenderTarget& renderTarget = _context.renderTargetPool().renderTarget(params._target);
        const Texture_ptr& DepthTex = renderTarget.getAttachment(RTAttachmentType::Depth, 0).texture();

        GFX::BindDescriptorSetsCommand bindDescriptorSets = {};
        bindDescriptorSets._set._textureData.setTexture(DepthTex->getData(), to_U8(ShaderProgram::TextureUsage::DEPTH));
        GFX::EnqueueCommand(bufferInOut, bindDescriptorSets);
    }
}

void RenderPassManager::mainPass(const VisibleNodeList& nodes, const PassParams& params, RenderTarget& target, GFX::CommandBuffer& bufferInOut, bool prePassExecuted) {
    GFX::BeginDebugScopeCommand beginDebugScopeCmd;
    beginDebugScopeCmd._scopeID = 1;
    beginDebugScopeCmd._scopeName = " - MainPass";
    GFX::EnqueueCommand(bufferInOut, beginDebugScopeCmd);

    SceneManager& sceneManager = parent().sceneManager();

    RenderStagePass stagePass(params._stage, RenderPassType::MAIN_PASS, params._passVariant, params._passIndex);

    prepareRenderQueues(stagePass, params, nodes, !prePassExecuted, bufferInOut);

    if (params._target._usage != RenderTargetUsage::COUNT) {
        Attorney::SceneManagerRenderPass::preRenderMainPass(sceneManager, stagePass, *params._camera, params._target, bufferInOut);

        const bool hasLightingTarget = target.hasAttachment(RTAttachmentType::Colour, to_base(GFXDevice::ScreenTargets::EXTRA));
        GFX::BindDescriptorSetsCommand descriptorSetCmd = {};
        if (params._bindTargets) {
            const bool hasNormalsTarget = target.hasAttachment(RTAttachmentType::Colour, to_base(GFXDevice::ScreenTargets::NORMALS_AND_VELOCITY));

            // We don't need to clear the colour buffers at this stage since ... hopefully, they will be overwritten completely. Right?
            RTDrawDescriptor drawPolicy =  params._drawPolicy ? *params._drawPolicy
                                                              : RenderTarget::defaultPolicyNoClear();

            drawPolicy.enableState(RTDrawDescriptor::State::CLEAR_COLOUR_BUFFERS);
            drawPolicy.clearColour(to_U8(GFXDevice::ScreenTargets::ALBEDO), false);

            if (hasLightingTarget) {
                drawPolicy.clearColour(to_U8(GFXDevice::ScreenTargets::EXTRA), false);
                drawPolicy.drawMask().setEnabled(RTAttachmentType::Colour, to_base(GFXDevice::ScreenTargets::EXTRA), false);
            }

            if (hasNormalsTarget) {
                drawPolicy.clearColour(to_U8(GFXDevice::ScreenTargets::NORMALS_AND_VELOCITY), false);
                drawPolicy.drawMask().setEnabled(RTAttachmentType::Colour, to_base(GFXDevice::ScreenTargets::NORMALS_AND_VELOCITY), false);

                GFX::ResolveRenderTargetCommand resolveCmd = { };
                resolveCmd._source = params._target;
                resolveCmd._resolveColour = to_I8(GFXDevice::ScreenTargets::NORMALS_AND_VELOCITY);
                GFX::EnqueueCommand(bufferInOut, resolveCmd);

                const TextureData& data = target.getAttachmentPtr(RTAttachmentType::Colour, to_U8(GFXDevice::ScreenTargets::NORMALS_AND_VELOCITY))->texture()->getData();
                constexpr U8 bindSlot = to_U8(ShaderProgram::TextureUsage::NORMALMAP);
                descriptorSetCmd._set._textureData.setTexture(data, bindSlot);
            }

            if (prePassExecuted) {
                drawPolicy.drawMask().setEnabled(RTAttachmentType::Depth, 0, false);
            }
            
            GFX::BeginRenderPassCommand beginRenderPassCommand;
            beginRenderPassCommand._target = params._target;
            beginRenderPassCommand._descriptor = drawPolicy;
            beginRenderPassCommand._name = "DO_MAIN_PASS";
            GFX::EnqueueCommand(bufferInOut, beginRenderPassCommand);
        }

        if (hasLightingTarget) {
            GFX::ResolveRenderTargetCommand resolveCmd = { };
            resolveCmd._source = params._target;
            resolveCmd._resolveColour = to_I8(GFXDevice::ScreenTargets::EXTRA);
            GFX::EnqueueCommand(bufferInOut, resolveCmd);

            const TextureData& data = target.getAttachmentPtr(RTAttachmentType::Colour, to_U8(GFXDevice::ScreenTargets::EXTRA))->texture()->getData();
            constexpr U8 bindSlot = to_U8(ShaderProgram::TextureUsage::PREPASS_SHADOWS);

            descriptorSetCmd._set._textureData.setTexture(data, bindSlot);
        }

        GFX::EnqueueCommand(bufferInOut, descriptorSetCmd);
        // We try and render translucent items in the shadow pass and due some alpha-discard tricks
        renderQueueToSubPasses(stagePass, bufferInOut);

        Attorney::SceneManagerRenderPass::postRender(sceneManager, stagePass, *params._camera, bufferInOut);

        if (params._stage == RenderStage::DISPLAY) {
            /// These should be OIT rendered as well since things like debug nav meshes have translucency
            Attorney::SceneManagerRenderPass::debugDraw(sceneManager, stagePass, *params._camera, bufferInOut);
        }

        if (params._bindTargets) {
            GFX::EndRenderPassCommand endRenderPassCommand;
            GFX::EnqueueCommand(bufferInOut, endRenderPassCommand);
        }
    }

    GFX::EndDebugScopeCommand endDebugScopeCmd;
    GFX::EnqueueCommand(bufferInOut, endDebugScopeCmd);
}

void RenderPassManager::woitPass(const VisibleNodeList& nodes, const PassParams& params, const RenderTarget& target, GFX::CommandBuffer& bufferInOut) {
    PipelineDescriptor pipelineDescriptor;
    pipelineDescriptor._stateHash = _context.get2DStateBlock();
    pipelineDescriptor._shaderProgramHandle = _OITCompositionShader->getGUID();
    Pipeline* pipeline = _context.newPipeline(pipelineDescriptor);

    RenderStagePass stagePass(params._stage, RenderPassType::OIT_PASS, params._passVariant, params._passIndex);
    prepareRenderQueues(stagePass, params, nodes, false, bufferInOut);

    GFX::BeginDebugScopeCommand beginDebugScopeCmd;
    beginDebugScopeCmd._scopeID = 2;
    beginDebugScopeCmd._scopeName = " - W-OIT Pass";
    GFX::EnqueueCommand(bufferInOut, beginDebugScopeCmd);

    if (renderQueueSize(stagePass) > 0) {
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

            /*RTBlendState& state2 = beginRenderPassOitCmd._descriptor.blendState(to_U8(GFXDevice::ScreenTargets::EXTRA));
            state2._blendProperties._enabled = true;
            state2._blendProperties._blendSrc = BlendProperty::ONE;
            state2._blendProperties._blendDest = BlendProperty::ONE;
            state2._blendProperties._blendOp = BlendOperation::ADD;*/
        }

        // Don't clear our screen target. That would be BAD.
        beginRenderPassOitCmd._descriptor.clearColour(to_U8(GFXDevice::ScreenTargets::MODULATE), false);

        // Don't clear and don't write to depth buffer
        beginRenderPassOitCmd._descriptor.disableState(RTDrawDescriptor::State::CLEAR_DEPTH_BUFFER);
        beginRenderPassOitCmd._descriptor.drawMask().setEnabled(RTAttachmentType::Depth, 0, false);
        GFX::EnqueueCommand(bufferInOut, beginRenderPassOitCmd);

        renderQueueToSubPasses(stagePass, bufferInOut/*, quality*/);

        GFX::EndRenderPassCommand endRenderPassOitCmd;
        endRenderPassOitCmd._autoResolveMSAAColour = true;
        endRenderPassOitCmd._autoResolveMSAAExternalColour = false;
        endRenderPassOitCmd._autoResolveMSAADepth = false;
        GFX::EnqueueCommand(bufferInOut, endRenderPassOitCmd);

        // Step2: Composition pass
        // Don't clear depth & colours and do not write to the depth buffer

        GFX::SetCameraCommand setCameraCommand;
        setCameraCommand._cameraSnapshot = Camera::utilityCamera(Camera::UtilityCamera::_2D)->snapshot();
        setCameraCommand._stage = params._stage;
        GFX::EnqueueCommand(bufferInOut, setCameraCommand);

        GFX::BeginRenderPassCommand beginRenderPassCompCmd;
        beginRenderPassCompCmd._name = "DO_OIT_PASS_2";
        beginRenderPassCompCmd._target = params._target;
        beginRenderPassCompCmd._descriptor.disableState(RTDrawDescriptor::State::CLEAR_DEPTH_BUFFER);
        beginRenderPassCompCmd._descriptor.disableState(RTDrawDescriptor::State::CLEAR_COLOUR_BUFFERS);
        beginRenderPassCompCmd._descriptor.drawMask().setEnabled(RTAttachmentType::Depth, 0, false);
        {
            RTBlendState& state0 = beginRenderPassCompCmd._descriptor.blendState(to_U8(GFXDevice::ScreenTargets::ALBEDO));
            state0._blendProperties._enabled = true;
            state0._blendProperties._blendOp = BlendOperation::ADD;
#if defined(USE_COLOUR_WOIT)
            state0._blendProperties._blendSrc = BlendProperty::INV_SRC_ALPHA;
            state0._blendProperties._blendDest = BlendProperty::ONE;
#else
            state0._blendProperties._blendSrc = BlendProperty::SRC_ALPHA;
            state0._blendProperties._blendDest = BlendProperty::INV_SRC_ALPHA;
#endif
        }
        GFX::EnqueueCommand(bufferInOut, beginRenderPassCompCmd);

        GFX::BindPipelineCommand bindPipelineCmd;
        bindPipelineCmd._pipeline = pipeline;
        GFX::EnqueueCommand(bufferInOut, bindPipelineCmd);

        RenderTarget& oitTarget = _context.renderTargetPool().renderTarget(RenderTargetID(RenderTargetUsage::OIT));
        TextureData accum = oitTarget.getAttachment(RTAttachmentType::Colour, to_U8(GFXDevice::ScreenTargets::ACCUMULATION)).texture()->getData();
        TextureData revealage = oitTarget.getAttachment(RTAttachmentType::Colour, to_U8(GFXDevice::ScreenTargets::REVEALAGE)).texture()->getData();

        GFX::BindDescriptorSetsCommand descriptorSetCmd;
        descriptorSetCmd._set._textureData.setTexture(accum, to_base(ShaderProgram::TextureUsage::UNIT0));
        descriptorSetCmd._set._textureData.setTexture(revealage, to_base(ShaderProgram::TextureUsage::UNIT1));
        GFX::EnqueueCommand(bufferInOut, descriptorSetCmd);

        GenericDrawCommand drawCommand;
        drawCommand._primitiveType = PrimitiveType::TRIANGLES;

        GFX::DrawCommand drawCmd = { drawCommand };
        GFX::EnqueueCommand(bufferInOut, drawCmd);

        GFX::EndRenderPassCommand endRenderPassCompCmd;
        endRenderPassCompCmd._autoResolveMSAAColour = true;
        GFX::EnqueueCommand(bufferInOut, endRenderPassCompCmd);
    }

    GFX::EndDebugScopeCommand endDebugScopeCmd;
    GFX::EnqueueCommand(bufferInOut, endDebugScopeCmd);
}

void RenderPassManager::doCustomPass(PassParams& params, GFX::CommandBuffer& bufferInOut) {
    Attorney::SceneManagerRenderPass::prepareLightData(parent().sceneManager(), params._stage, *params._camera);

        // Cull the scene and grab the visible nodes
    const VisibleNodeList& visibleNodes = Attorney::SceneManagerRenderPass::cullScene(parent().sceneManager(), params._stage, *params._camera, params._minLoD, params._minExtents);

    // Tell the Rendering API to draw from our desired PoV
    GFX::SetCameraCommand setCameraCommand;
    setCameraCommand._cameraSnapshot = params._camera->snapshot();
    setCameraCommand._stage = params._stage;
    GFX::EnqueueCommand(bufferInOut, setCameraCommand);

    GFX::SetClipPlanesCommand setClipPlanesCommand;
    setClipPlanesCommand._clippingPlanes = params._clippingPlanes;
    GFX::EnqueueCommand(bufferInOut, setClipPlanesCommand);

    RenderTarget& target = _context.renderTargetPool().renderTarget(params._target);

    GFX::BeginDebugScopeCommand beginDebugScopeCmd = {};
    beginDebugScopeCmd._scopeID = 0;
    beginDebugScopeCmd._scopeName = Util::StringFormat("Custom pass ( %s )", TypeUtil::renderStageToString(params._stage)).c_str();
    GFX::EnqueueCommand(bufferInOut, beginDebugScopeCmd);

    GFX::BindDescriptorSetsCommand bindDescriptorSets = {};
    bindDescriptorSets._set._textureData.setTexture(_context.getPrevDepthBuffer()->getData(), to_U8(ShaderProgram::TextureUsage::DEPTH_PREV));
    GFX::EnqueueCommand(bufferInOut, bindDescriptorSets);

    bool prePassExecuted = prePass(visibleNodes, params, target, bufferInOut);
    occlusionPass(visibleNodes, params, target, bufferInOut, prePassExecuted);
    mainPass(visibleNodes, params, target, bufferInOut, prePassExecuted);

    if (params._stage != RenderStage::SHADOW) {

        GFX::ResolveRenderTargetCommand resolveCmd = { };
        resolveCmd._source = params._target;
        resolveCmd._resolveColours = true;
        GFX::EnqueueCommand(bufferInOut, resolveCmd);

        woitPass(visibleNodes, params, target, bufferInOut);
    }

    GFX::EndDebugScopeCommand endDebugScopeCmd;
    GFX::EnqueueCommand(bufferInOut, endDebugScopeCmd);
}


// TEMP
U32 RenderPassManager::renderQueueSize(RenderStagePass stagePass, RenderPackage::MinQuality qualityRequirement) const {
    const vectorEASTLFast<RenderPackage*>& queue = _renderQueues[to_base(stagePass._stage)];
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

void RenderPassManager::renderQueueToSubPasses(RenderStagePass stagePass, GFX::CommandBuffer& commandsInOut, RenderPackage::MinQuality qualityRequirement) const {
    const vectorEASTLFast<RenderPackage*>& queue = _renderQueues[to_base(stagePass._stage)];

    if (qualityRequirement == RenderPackage::MinQuality::COUNT) {
        for (RenderPackage* item : queue) {
            Attorney::RenderPackageRenderPassManager::getCommandBuffer(*item, commandsInOut);
        }
    } else {
        for (RenderPackage* item : queue) {
            if (item->qualityRequirement() == qualityRequirement) {
                Attorney::RenderPackageRenderPassManager::getCommandBuffer(*item, commandsInOut);
            }
        }
    }
}

};