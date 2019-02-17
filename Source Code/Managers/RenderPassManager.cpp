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


//#define DISABLE_WOIT

#ifndef USE_COLOUR_WOIT
//#define USE_COLOUR_WOIT
#endif

namespace Divide {

namespace {
    thread_local RenderQueue::SortedQueues g_sortedQueues;
    thread_local vectorEASTL<GFXDevice::NodeData> g_nodeData;
    thread_local vectorEASTL<IndirectDrawCommand> g_drawCommands;
};

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
    ResourceDescriptor shaderDesc("OITComposition");
    _OITCompositionShader = CreateResource<ShaderProgram>(parent.resourceCache(), shaderDesc);
    _renderPassCommandBuffer.push_back(GFX::allocateCommandBuffer(true));
    _mainCommandBuffer = GFX::allocateCommandBuffer();
}

RenderPassManager::~RenderPassManager()
{
    GFX::deallocateCommandBuffer(_mainCommandBuffer);
    for (GFX::CommandBuffer*& buf : _renderPassCommandBuffer) {
        GFX::deallocateCommandBuffer(buf, true);
    }
}

void RenderPassManager::render(SceneRenderState& sceneRenderState, Time::ProfileTimer* parentTimer) {
    const Configuration& config = parent().platformContext().config();

    if (parentTimer != nullptr && !parentTimer->hasChildTimer(*_renderPassTimer)) {
        parentTimer->addChildTimer(*_renderPassTimer);
        parentTimer->addChildTimer(*_postFxRenderTimer);
        parentTimer->addChildTimer(*_flushCommandBufferTimer);
    }

    const Camera& cam = Attorney::SceneManagerRenderPass::playerCamera(parent().sceneManager());

    Attorney::SceneManagerRenderPass::preRenderAllPasses(parent().sceneManager(), cam);

    TaskPriority priority = config.rendering.multithreadedCommandGeneration ? TaskPriority::DONT_CARE : TaskPriority::REALTIME;

    TaskPool& pool = parent().platformContext().taskPool(TaskPoolType::HIGH_PRIORITY);

    U8 renderPassCount = to_U8(_renderPasses.size());

    std::atomic_uint remainingTasks = renderPassCount;
    {
        Time::ScopedTimer timeAll(*_renderPassTimer);

        for (U8 i = 0; i < renderPassCount; ++i) {

            RenderPass* pass = _renderPasses[i].get();
            GFX::CommandBuffer* buf = _renderPassCommandBuffer[i];

            CreateTask(pool,
                       nullptr,
                       [pass, buf, &sceneRenderState, &remainingTasks](const Task& parentTask) {
                           assert(buf->empty());
                           pass->render(sceneRenderState, *buf);
                           buf->batch();
                           remainingTasks.fetch_sub(1);
                       }).startTask(priority);
        }

        GFX::CommandBuffer* buf = _renderPassCommandBuffer.back();
        {
            Time::ScopedTimer time(*_postFxRenderTimer);
            assert(buf->empty());
            parent().platformContext().gfx().postFX().apply(cam, *buf);
            buf->batch();
        }

        while(remainingTasks.load() > 0) {
            parent().idle();
            std::this_thread::yield();
        }
    }

    if (config.rendering.batchPassBuffers) {
        {            
            Time::ScopedTimer timeCommands(*_buildCommandBufferTimer);
            _mainCommandBuffer->clear(false);
            for (GFX::CommandBuffer* buf : _renderPassCommandBuffer) {
                _mainCommandBuffer->addDestructive(*buf);
            }
        }

        Time::ScopedTimer timeCommands(*_flushCommandBufferTimer);
        _context.flushCommandBuffer(*_mainCommandBuffer);
        
    } else {
        {
            Time::ScopedTimer timeCommands(*_buildCommandBufferTimer);
        }

        Time::ScopedTimer timeCommands(*_flushCommandBufferTimer);
        for (U8 i = 0; i < renderPassCount + 1; ++i) {
            _context.flushCommandBuffer(*_renderPassCommandBuffer[i]);
        }
        for (U8 i = 0; i < renderPassCount + 1; ++i) {
            _renderPassCommandBuffer[i]->clear(false);
        }
    }

    for (U8 i = 0; i < renderPassCount; ++i) {
        _renderPasses[i]->postRender();
    }

    Attorney::SceneManagerRenderPass::postRenderAllPasses(parent().sceneManager(), cam);
}

RenderPass& RenderPassManager::addRenderPass(const stringImpl& renderPassName,
                                             U8 orderKey,
                                             RenderStage renderStage) {
    assert(!renderPassName.empty());

    _renderPasses.push_back(std::make_shared<RenderPass>(*this, _context, renderPassName, orderKey, renderStage));
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
GFXDevice::NodeData RenderPassManager::processVisibleNode(SceneGraphNode* node, RenderStagePass stagePass, bool isOcclusionCullable, bool playAnimations, const mat4<F32>& viewMatrix) const {
    GFXDevice::NodeData dataOut;

    // Extract transform data (if available)
    // (Nodes without transforms just use identity matrices)
    TransformComponent* const transform = node->get<TransformComponent>();
    if (transform) {
        // ... get the node's world matrix properly interpolated
        dataOut._worldMatrix.set(transform->getWorldMatrix(_context.getFrameInterpolationFactor()));
        dataOut._normalMatrixW.set(dataOut._worldMatrix);
        if (!transform->isUniformScaled()) {
            // Non-uniform scaling requires an inverseTranspose to negate
            // scaling contribution but preserve rotation
            dataOut._normalMatrixW.setRow(3, 0.0f, 0.0f, 0.0f, 1.0f);
            dataOut._normalMatrixW.inverseTranspose();
        }
    }

    dataOut._normalMatrixW.setRow(3, 0.0f, 0.0f, 0.0f, 0.0f);

    // Get the material property matrix (alpha test, texture count, texture operation, etc.)
    AnimationComponent* animComp = nullptr;
    if (playAnimations) {
        animComp = node->get<AnimationComponent>();
        if (animComp && animComp->playAnimations()) {
            dataOut._normalMatrixW.element(0, 3) = to_F32(animComp->boneCount());
        }
    }

    RenderingComponent* const renderable = node->get<RenderingComponent>();
    renderable->getRenderingProperties(stagePass,
                                       dataOut._properties,
                                       dataOut._normalMatrixW.element(1, 3),
                                       dataOut._normalMatrixW.element(2, 3));

    // Since the normal matrix is 3x3, we can use the extra row and column to store additional data
    BoundsComponent* const bounds = node->get<BoundsComponent>();
    dataOut._normalMatrixW.setRow(3, bounds->getBoundingSphere().asVec4());

    // Get the colour matrix (diffuse, specular, etc.)
    renderable->getMaterialColourMatrix(dataOut._colourMatrix);

    // Temp: Make the hovered/selected node brighter. 
    if (dataOut._properties.x > 0.5f || dataOut._properties.x < -0.5f) {
        FColour matColour = dataOut._colourMatrix.getRow(0);
        if (dataOut._properties.x < -0.5f) {
            matColour *= 3;
        } else {
            matColour *= 2;
        }
        dataOut._colourMatrix.setRow(0, matColour);
    }

    //set properties.w to -1 to skip occlusion culling for the node
    dataOut._properties.w = isOcclusionCullable ? 1.0f : -1.0f;

    return dataOut;
}

void RenderPassManager::refreshNodeData(RenderStagePass stagePass,
                                        const SceneRenderState& renderState,
                                        const mat4<F32>& viewMatrix,
                                        const RenderQueue::SortedQueues& sortedQueues,
                                        GFX::CommandBuffer& bufferInOut)
{
    bool playAnimations = renderState.isEnabledOption(SceneRenderState::RenderOptions::PLAY_ANIMATIONS);

    g_nodeData.resize(0);
    g_nodeData.reserve(Config::MAX_VISIBLE_NODES);

    g_drawCommands.resize(0);
    g_drawCommands.reserve(Config::MAX_VISIBLE_NODES);

    for (const vectorEASTLFast<SceneGraphNode*>& queue : sortedQueues) {
        for (SceneGraphNode* node : queue) {
            RenderingComponent& renderable = *node->get<RenderingComponent>();

            RefreshNodeDataParams params(g_drawCommands, bufferInOut);
            params._stagePass = stagePass;
            params._nodeCount = to_U32(g_nodeData.size());

            if (Attorney::RenderingCompRenderPass::onRefreshNodeData(renderable, params)) {
                g_nodeData.push_back(processVisibleNode(node, stagePass, renderable.renderOptionEnabled(RenderingComponent::RenderOptions::IS_OCCLUSION_CULLABLE), playAnimations, viewMatrix));
            }
        }
    }

    U32 nodeCount = to_U32(g_nodeData.size());
    U32 cmdCount = to_U32(g_drawCommands.size());
    assert(cmdCount >= nodeCount);

    RenderPass::BufferData bufferData = getBufferData(stagePass);
    *bufferData._lastCommandCount = cmdCount;

    bufferData._cmdBuffer->writeData(0, cmdCount, (bufferPtr)g_drawCommands.data());
    bufferData._renderData->writeData(bufferData._renderDataElementOffset, nodeCount, (bufferPtr)g_nodeData.data());

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

            Attorney::RenderingCompRenderPass::prepareDrawPackage(*node->get<RenderingComponent>(), *params._camera, sceneRenderState, stagePass);
        }
    }

    if (refresh) {
        GFX::MemoryBarrierCommand memCmd;
        memCmd._barrierMask = to_base(MemoryBarrierType::SHADER_BUFFER);
        GFX::EnqueueCommand(bufferInOut, memCmd);

        refreshNodeData(stagePass, sceneRenderState, params._camera->getViewMatrix(), g_sortedQueues, bufferInOut);
    }
}

void RenderPassManager::prepareRenderQueues(RenderStagePass stagePass, const PassParams& params, bool refreshNodeData, GFX::CommandBuffer& bufferInOut) {
    RenderStage stage = stagePass._stage;

    const RenderPassCuller::VisibleNodeList& visibleNodes = refreshNodeData ? Attorney::SceneManagerRenderPass::cullScene(parent().sceneManager(), stage, *params._camera, params._minLoD)
                                                                            : Attorney::SceneManagerRenderPass::getVisibleNodesCache(parent().sceneManager(), stage);

    RenderQueue& queue = getQueue();
    queue.refresh(stage);
    for (const RenderPassCuller::VisibleNode& node : visibleNodes) {
        queue.addNodeToQueue(*node._node, stagePass, node._distanceToCameraSq);
    }
    // Sort all bins
    queue.sort(stagePass);
    
    vectorEASTLFast<RenderPackage*>& packageQueue = _renderQueues[to_base(stage)];
    packageQueue.resize(0);
    packageQueue.reserve(Config::MAX_VISIBLE_NODES);
    
    if (stagePass._passType == RenderPassType::PRE_PASS || stagePass._stage == RenderStage::SHADOW) {
        // Draw everything in the depth pass
        queue.populateRenderQueues(stagePass,RenderBinType::RBT_COUNT, packageQueue);
    } else {
#if defined(DISABLE_WOIT)
        queue.populateRenderQueues(stagePass, RenderBinType::RBT_COUNT, packageQueue);
#else
        // Only draw stuff from the translucent bin in the OIT Pass and everything else in the colour pass
        bool oitPass = stagePass._passType == RenderPassType::OIT_PASS;
        queue.populateRenderQueues(stagePass, oitPass ? RenderBinType::RBT_TRANSLUCENT : RenderBinType::RBT_COUNT, packageQueue);
#endif
    }

    buildDrawCommands(stagePass, params, refreshNodeData, bufferInOut);
}

bool RenderPassManager::prePass(const PassParams& params, const RenderTarget& target, GFX::CommandBuffer& bufferInOut) {
    // PrePass requires a depth buffer
    bool doPrePass = params._stage != RenderStage::SHADOW &&
                     params._target._usage != RenderTargetUsage::COUNT &&
                     target.getAttachment(RTAttachmentType::Depth, 0).used();

    if (doPrePass) {

        GFX::BeginDebugScopeCommand beginDebugScopeCmd;
        beginDebugScopeCmd._scopeID = 0;
        beginDebugScopeCmd._scopeName = " - PrePass";
        GFX::EnqueueCommand(bufferInOut, beginDebugScopeCmd);

        RenderStagePass stagePass(params._stage, RenderPassType::PRE_PASS, params._passVariant, params._passIndex);
        prepareRenderQueues(stagePass, params, true, bufferInOut);

        if (params._bindTargets) {
            GFX::BeginRenderPassCommand beginRenderPassCommand;
            beginRenderPassCommand._target = params._target;
            beginRenderPassCommand._descriptor = RenderTarget::defaultPolicyDepthOnly();
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

void RenderPassManager::mainPass(const PassParams& params, RenderTarget& target, GFX::CommandBuffer& bufferInOut, bool prePassExecuted) {
    GFX::BeginDebugScopeCommand beginDebugScopeCmd;
    beginDebugScopeCmd._scopeID = 1;
    beginDebugScopeCmd._scopeName = " - MainPass";
    GFX::EnqueueCommand(bufferInOut, beginDebugScopeCmd);

    SceneManager& sceneManager = parent().sceneManager();

    RenderStagePass stagePass(params._stage, RenderPassType::MAIN_PASS, params._passVariant, params._passIndex);

    prepareRenderQueues(stagePass, params, !prePassExecuted, bufferInOut);

    if (params._target._usage != RenderTargetUsage::COUNT) {
        Attorney::SceneManagerRenderPass::preRender(sceneManager, stagePass, *params._camera, target, bufferInOut);
    
        if (params._stage != RenderStage::SHADOW) {
            GFX::BindDescriptorSetsCommand bindDescriptorSets;
            // Bind the depth buffers
            TextureData depthBufferTextureData = target.getAttachment(RTAttachmentType::Depth, 0).texture()->getData();
            bindDescriptorSets._set._textureData.setTexture(depthBufferTextureData, to_U8(ShaderProgram::TextureUsage::DEPTH));

            TextureData prevDepthData = depthBufferTextureData;
            if (target.hasAttachment(RTAttachmentType::Colour, to_U8(GFXDevice::ScreenTargets::NORMALS_AND_VELOCITY))) {
                const RTAttachment_ptr& velocityAttachment = target.getAttachmentPtr(RTAttachmentType::Colour, to_U8(GFXDevice::ScreenTargets::NORMALS_AND_VELOCITY));
                const Texture_ptr& prevDepthTexture = _context.getPrevDepthBuffer();
                prevDepthData = (velocityAttachment->used() && prevDepthTexture) ? prevDepthTexture->getData() : depthBufferTextureData;
            }
            
            bindDescriptorSets._set._textureData.setTexture(prevDepthData, to_U8(ShaderProgram::TextureUsage::DEPTH_PREV));

            GFX::EnqueueCommand(bufferInOut, bindDescriptorSets);
        }

        if (params._bindTargets) {
            // We don't need to clear the colour buffers at this stage since ... hopefully, they will be overwritten completely. Right?
            RTDrawDescriptor drawPolicy =  params._drawPolicy ? *params._drawPolicy
                                                               : RenderTarget::defaultPolicyNoClear();

            drawPolicy.drawMask().setEnabled(RTAttachmentType::Depth, 0, false);

            
            GFX::BeginRenderPassCommand beginRenderPassCommand;
            beginRenderPassCommand._target = params._target;
            beginRenderPassCommand._descriptor = drawPolicy;
            beginRenderPassCommand._name = "DO_MAIN_PASS";
            GFX::EnqueueCommand(bufferInOut, beginRenderPassCommand);
        }

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

void RenderPassManager::woitPass(const PassParams& params, const RenderTarget& target, GFX::CommandBuffer& bufferInOut) {
    PipelineDescriptor pipelineDescriptor;
    pipelineDescriptor._stateHash = _context.get2DStateBlock();
    pipelineDescriptor._shaderProgramHandle = _OITCompositionShader->getID();
    Pipeline* pipeline = _context.newPipeline(pipelineDescriptor);

    RenderStagePass stagePass(params._stage, RenderPassType::OIT_PASS, params._passVariant, params._passIndex);
    prepareRenderQueues(stagePass, params, false, bufferInOut);

    // Weighted Blended Order Independent Transparency
    for (U8 i = 0; i < /*2*/1; ++i) {
        GFX::BeginDebugScopeCommand beginDebugScopeCmd;
        beginDebugScopeCmd._scopeID = 2;
        beginDebugScopeCmd._scopeName = Util::StringFormat(" - W-OIT Pass %d", to_U32(i)).c_str();
        GFX::EnqueueCommand(bufferInOut, beginDebugScopeCmd);

        //RenderPackage::MinQuality quality = i == 0 ? RenderPackage::MinQuality::FULL : RenderPackage::MinQuality::LOW;
        if (renderQueueSize(stagePass) > 0) {
            RenderTargetUsage rtUsage = i == 0 ? RenderTargetUsage::OIT_FULL_RES : RenderTargetUsage::OIT_QUARTER_RES;

            RenderTarget& oitTarget = _context.renderTargetPool().renderTarget(RenderTargetID(rtUsage));

            // Step1: Draw translucent items into the accumulation and revealage buffers
            GFX::BeginRenderPassCommand beginRenderPassOitCmd;
            beginRenderPassOitCmd._name = Util::StringFormat("DO_OIT_PASS_1_%s", (i == 0 ? "HIGH" : "LOW")).c_str();
            beginRenderPassOitCmd._target = RenderTargetID(rtUsage);
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
            }
            // Don't clear our screen target. That would be BAD.
            beginRenderPassOitCmd._descriptor.clearColour(to_U8(GFXDevice::ScreenTargets::MODULATE), false);
            // Don't clear and don't write to depth buffer
            beginRenderPassOitCmd._descriptor.disableState(RTDrawDescriptor::State::CLEAR_DEPTH_BUFFER);
            beginRenderPassOitCmd._descriptor.drawMask().setEnabled(RTAttachmentType::Depth, 0, false);
            GFX::EnqueueCommand(bufferInOut, beginRenderPassOitCmd);

            renderQueueToSubPasses(stagePass, bufferInOut/*, quality*/);

            GFX::EndRenderPassCommand endRenderPassOitCmd;
            GFX::EnqueueCommand(bufferInOut, endRenderPassOitCmd);

            // Step2: Composition pass
            // Don't clear depth & colours and do not write to the depth buffer

            GFX::SetCameraCommand setCameraCommand;
            setCameraCommand._cameraSnapshot = Camera::utilityCamera(Camera::UtilityCamera::_2D)->snapshot();
            GFX::EnqueueCommand(bufferInOut, setCameraCommand);

            GFX::BeginRenderPassCommand beginRenderPassCompCmd;
            beginRenderPassCompCmd._name = Util::StringFormat("DO_OIT_PASS_2_%s", (i == 0 ? "HIGH" : "LOW")).c_str();
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

            TextureData accum = oitTarget.getAttachment(RTAttachmentType::Colour, to_U8(GFXDevice::ScreenTargets::ACCUMULATION)).texture()->getData();
            TextureData revealage = oitTarget.getAttachment(RTAttachmentType::Colour, to_U8(GFXDevice::ScreenTargets::REVEALAGE)).texture()->getData();

            GFX::BindDescriptorSetsCommand descriptorSetCmd;
            descriptorSetCmd._set._textureData.setTexture(accum, to_base(ShaderProgram::TextureUsage::UNIT0));
            descriptorSetCmd._set._textureData.setTexture(revealage, to_base(ShaderProgram::TextureUsage::UNIT1));
            GFX::EnqueueCommand(bufferInOut, descriptorSetCmd);

            GenericDrawCommand drawCommand;
            drawCommand._primitiveType = PrimitiveType::TRIANGLES;
            drawCommand._drawCount = 1;

            GFX::DrawCommand drawCmd;
            drawCmd._drawCommands.push_back(drawCommand);
            GFX::EnqueueCommand(bufferInOut, drawCmd);

            GFX::EndRenderPassCommand endRenderPassCompCmd;
            GFX::EnqueueCommand(bufferInOut, endRenderPassCompCmd);
        }

        GFX::EndDebugScopeCommand endDebugScopeCmd;
        GFX::EnqueueCommand(bufferInOut, endDebugScopeCmd);
    }
}

void RenderPassManager::doCustomPass(PassParams& params, GFX::CommandBuffer& bufferInOut) {
    Attorney::SceneManagerRenderPass::prepareLightData(parent().sceneManager(), params._stage, *params._camera);

    // Tell the Rendering API to draw from our desired PoV
    GFX::SetCameraCommand setCameraCommand;
    setCameraCommand._cameraSnapshot = params._camera->snapshot();
    GFX::EnqueueCommand(bufferInOut, setCameraCommand);

    GFX::SetClipPlanesCommand setClipPlanesCommand;
    setClipPlanesCommand._clippingPlanes = params._clippingPlanes;
    GFX::EnqueueCommand(bufferInOut, setClipPlanesCommand);

    RenderTarget& target = _context.renderTargetPool().renderTarget(params._target);

    GFX::BeginDebugScopeCommand beginDebugScopeCmd;
    beginDebugScopeCmd._scopeID = 0;
    beginDebugScopeCmd._scopeName = Util::StringFormat("Custom pass ( %s )", TypeUtil::renderStageToString(params._stage)).c_str();
    GFX::EnqueueCommand(bufferInOut, beginDebugScopeCmd);

    bool prePassExecuted = prePass(params, target, bufferInOut);

    if (prePassExecuted && params._occlusionCull) {
        const Texture_ptr& HiZTex = _context.constructHIZ(params._target, bufferInOut);
        _context.occlusionCull(getBufferData(RenderStagePass(params._stage, RenderPassType::PRE_PASS, params._passVariant, params._passIndex)),
                               HiZTex,
                               params._camera->getZPlanes(),
                               bufferInOut);
        if (params._stage == RenderStage::DISPLAY) {
            GFX::MemoryBarrierCommand memCmd;
            memCmd._barrierMask = to_base(MemoryBarrierType::COUNTER);
            GFX::EnqueueCommand(bufferInOut, memCmd);

            _context.updateCullCount(bufferInOut);
        }
    }

    mainPass(params, target, bufferInOut, prePassExecuted);

#if !defined(DISABLE_WOIT)
    if (params._stage != RenderStage::SHADOW) {
        woitPass(params, target, bufferInOut);
    }
#endif

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