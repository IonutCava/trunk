#include "stdafx.h"

#include "Headers/RenderPassManager.h"

#include "Core/Headers/Kernel.h"
#include "Core/Headers/TaskPool.h"
#include "Core/Headers/StringHelper.h"
#include "Core/Headers/PlatformContext.h"
#include "Managers/Headers/SceneManager.h"
#include "Rendering/Camera/Headers/Camera.h"
#include "Rendering/RenderPass/Headers/RenderQueue.h"
#include "Platform/Video/Textures/Headers/Texture.h"

#ifndef USE_COLOUR_WOIT
//#define USE_COLOUR_WOIT
#endif

namespace Divide {

RenderPassManager::RenderPassManager(Kernel& parent, GFXDevice& context)
    : KernelComponent(parent),
      _context(context),
      _renderQueue(context)
{
    ResourceDescriptor shaderDesc("OITComposition");
    _OITCompositionShader = CreateResource<ShaderProgram>(parent.resourceCache(), shaderDesc);
}

RenderPassManager::~RenderPassManager()
{
    for (GFX::CommandBuffer*& buf : _renderPassCommandBuffer) {
        GFX::deallocateCommandBuffer(buf);
    }
}

void RenderPassManager::render(SceneRenderState& sceneRenderState) {
    TaskPool& pool = parent().platformContext().taskPool();
    TaskHandle renderTarsk = CreateTask(pool, DELEGATE_CBK<void, const Task&>());

    for (vec_size_eastl i = 0; i < _renderPasses.size(); ++i) {
        std::shared_ptr<RenderPass>& rp = _renderPasses[i];
        GFX::CommandBuffer& buf = *_renderPassCommandBuffer[i];
        CreateTask(pool,
            &renderTarsk,
            [&rp, &buf, &sceneRenderState](const Task& parentTask) {
                rp->render(sceneRenderState, buf);
            }).startTask(TaskPriority::REALTIME);
    }

    renderTarsk.startTask(TaskPriority::REALTIME).wait();

    
    //ToDo: Maybe handle this differently?
#if 1 //Direct render and clear
    for (vec_size_eastl i = 0; i < _renderPassCommandBuffer.size(); ++i) {
        _context.flushAndClearCommandBuffer(*_renderPassCommandBuffer[i]);
    }
#else // Maybe better batching and validation?
    GFX::ScopedCommandBuffer sBuffer(GFX::allocateScopedCommandBuffer());
    for (vec_size_eastl i = 0; i < _renderPassCommandBuffer.size(); ++i) {
        GFX::CommandBuffer& buf = *_renderPassCommandBuffer[i];
        sBuffer().add(buf);
        buf.clear();
    }
    _context.flushCommandBuffer(sBuffer());
#endif
}

RenderPass& RenderPassManager::addRenderPass(const stringImpl& renderPassName,
                                             U8 orderKey,
                                             RenderStage renderStage) {
    assert(!renderPassName.empty());

    _renderPasses.push_back(std::make_shared<RenderPass>(*this, _context, renderPassName, orderKey, renderStage));
    _renderPassCommandBuffer.push_back(GFX::allocateCommandBuffer());

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
            GFX::deallocateCommandBuffer(buf);
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

const RenderPass::BufferData& 
RenderPassManager::getBufferData(RenderStage renderStage, I32 bufferIndex) const {
    return getPassForStage(renderStage).getBufferData(bufferIndex);
}

RenderPass::BufferData&
RenderPassManager::getBufferData(RenderStage renderStage, I32 bufferIndex) {
    return getPassForStage(renderStage).getBufferData(bufferIndex);
}

void RenderPassManager::prePass(const PassParams& params, const RenderTarget& target, GFX::CommandBuffer& bufferInOut) {
    GFX::BeginDebugScopeCommand beginDebugScopeCmd;
    beginDebugScopeCmd._scopeID = 0;
    beginDebugScopeCmd._scopeName = Util::StringFormat("Custom pass ( %s ): PrePass", TypeUtil::renderStageToString(params._stage)).c_str();
    GFX::EnqueueCommand(bufferInOut, beginDebugScopeCmd);

    // PrePass requires a depth buffer
    bool doPrePass = params._doPrePass && target.getAttachment(RTAttachmentType::Depth, 0).used();

    if (doPrePass) {
        SceneManager& sceneManager = parent().sceneManager();

        RenderStagePass stagePass(params._stage, RenderPassType::DEPTH_PASS);

        Attorney::SceneManagerRenderPass::populateRenderQueue(sceneManager,
                                                              stagePass,
                                                              *params._camera,
                                                              true,
                                                              params._pass);

        if (params._target._usage != RenderTargetUsage::COUNT) {
           
            if (params._bindTargets) {
                GFX::BeginRenderPassCommand beginRenderPassCommand;
                beginRenderPassCommand._target = params._target;
                beginRenderPassCommand._descriptor = RenderTarget::defaultPolicyDepthOnly();
                beginRenderPassCommand._name = "DO_PRE_PASS";
                GFX::EnqueueCommand(bufferInOut, beginRenderPassCommand);
            }

            for (RenderBinType type : RenderBinType::_values()) {
                 _context.renderQueueToSubPasses(type, bufferInOut);
            }

            Attorney::SceneManagerRenderPass::postRender(sceneManager, stagePass, *params._camera, bufferInOut);

            if (params._bindTargets) {
                GFX::EndRenderPassCommand endRenderPassCommand;
                GFX::EnqueueCommand(bufferInOut, endRenderPassCommand);
            }
        }
        
    }

    GFX::EndDebugScopeCommand endDebugScopeCmd;
    GFX::EnqueueCommand(bufferInOut, endDebugScopeCmd);
}

void RenderPassManager::mainPass(const PassParams& params, RenderTarget& target, GFX::CommandBuffer& bufferInOut) {
    static const vector<RenderBinType> shadowExclusionList
    {
        RenderBinType::RBT_DECAL,
        RenderBinType::RBT_SKY
    };

    GFX::BeginDebugScopeCommand beginDebugScopeCmd;
    beginDebugScopeCmd._scopeID = 1;
    beginDebugScopeCmd._scopeName = Util::StringFormat("Custom pass ( %s ): RenderPass", TypeUtil::renderStageToString(params._stage)).c_str();
    GFX::EnqueueCommand(bufferInOut, beginDebugScopeCmd);

    SceneManager& sceneManager = parent().sceneManager();

    RenderStagePass stagePass(params._stage, RenderPassType::COLOUR_PASS);
    Attorney::SceneManagerRenderPass::populateRenderQueue(sceneManager,
                                                          stagePass,
                                                          *params._camera,
                                                          !params._doPrePass,
                                                          params._pass);

    if (params._target._usage != RenderTargetUsage::COUNT) {
        bool drawToDepth = true;
        if (params._stage != RenderStage::SHADOW) {
            Attorney::SceneManagerRenderPass::preRender(sceneManager, stagePass, *params._camera, target, bufferInOut);
            if (params._doPrePass) {
                drawToDepth = Config::DEBUG_HIZ_CULLING;
            }
        }

        if (params._stage == RenderStage::DISPLAY) {
            GFX::BindDescriptorSetsCommand bindDescriptorSets;
            bindDescriptorSets._set = _context.newDescriptorSet();
            // Bind the depth buffers
            TextureData depthBufferTextureData = target.getAttachment(RTAttachmentType::Depth, 0).texture()->getData();
            bindDescriptorSets._set->_textureData.addTexture(depthBufferTextureData, to_U8(ShaderProgram::TextureUsage::DEPTH));

            const RTAttachment& velocityAttachment = target.getAttachment(RTAttachmentType::Colour, to_U8(GFXDevice::ScreenTargets::VELOCITY));
            if (velocityAttachment.used()) {
                const Texture_ptr& prevDepthTexture = _context.getPrevDepthBuffer();
                TextureData prevDepthData = (prevDepthTexture ? prevDepthTexture->getData() : depthBufferTextureData);
                bindDescriptorSets._set->_textureData.addTexture(prevDepthData, to_U8(ShaderProgram::TextureUsage::DEPTH_PREV));
            }

            GFX::EnqueueCommand(bufferInOut, bindDescriptorSets);
        }

        RTDrawDescriptor& drawPolicy = 
            params._drawPolicy ? *params._drawPolicy
                               : (!Config::DEBUG_HIZ_CULLING ? RenderTarget::defaultPolicyKeepDepth()
                                                             : RenderTarget::defaultPolicy());

        drawPolicy.drawMask().setEnabled(RTAttachmentType::Depth, 0, drawToDepth);

        if (params._bindTargets) {
            GFX::BeginRenderPassCommand beginRenderPassCommand;
            beginRenderPassCommand._target = params._target;
            beginRenderPassCommand._descriptor = drawPolicy;
            beginRenderPassCommand._name = "DO_MAIN_PASS";
            GFX::EnqueueCommand(bufferInOut, beginRenderPassCommand);
        }

        if (params._stage == RenderStage::SHADOW) {
            for (RenderBinType type : RenderBinType::_values()) {
                if (std::find(std::begin(shadowExclusionList),
                              std::end(shadowExclusionList),
                              type) == std::cend(shadowExclusionList))
                {
                    // We try and render translucent items in the shadow pass and due some alpha-discard tricks
                    _context.renderQueueToSubPasses(type, bufferInOut);
                }
            }
        } else {
            for (RenderBinType type : RenderBinType::_values()) {
               _context.renderQueueToSubPasses(type, bufferInOut);
            }
        }

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
    SceneManager& sceneManager = parent().sceneManager();

    RenderStagePass stagePass(params._stage, RenderPassType::OIT_PASS);
    Attorney::SceneManagerRenderPass::populateRenderQueue(sceneManager,
                                                          stagePass,
                                                          *params._camera,
                                                          false,
                                                          params._pass);

    // Weighted Blended Order Independent Transparency
    for (U8 i = 0; i < /*2*/1; ++i) {
        //RenderPackage::MinQuality quality = i == 0 ? RenderPackage::MinQuality::FULL : RenderPackage::MinQuality::LOW;
        if (_context.renderQueueSize(RenderBinType::RBT_TRANSLUCENT/*, quality*/) > 0) {
            RenderTargetUsage rtUsage = i == 0 ? RenderTargetUsage::OIT_FULL_RES : RenderTargetUsage::OIT_QUARTER_RES;

            RenderTarget& oitTarget = _context.renderTargetPool().renderTarget(RenderTargetID(rtUsage));

            // Step1: Draw translucent items into the accumulation and revealage buffers
            GFX::BeginRenderPassCommand beginRenderPassOitCmd;
            beginRenderPassOitCmd._name = Util::StringFormat("DO_OIT_PASS_1_%s", (i == 0 ? "HIGH" : "LOW")).c_str();
            beginRenderPassOitCmd._target = RenderTargetID(rtUsage);
            {
                RTBlendState& state0 = beginRenderPassOitCmd._descriptor.blendState(to_U8(GFXDevice::ScreenTargets::ACCUMULATION));
                state0._blendProperties._blendSrc = BlendProperty::ONE;
                state0._blendProperties._blendDest = BlendProperty::ONE;
                state0._blendProperties._blendOp = BlendOperation::ADD;

                RTBlendState& state1 = beginRenderPassOitCmd._descriptor.blendState(to_U8(GFXDevice::ScreenTargets::REVEALAGE));
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

            _context.renderQueueToSubPasses(RenderBinType::RBT_TRANSLUCENT/*, quality*/, bufferInOut);

            GFX::EndRenderPassCommand endRenderPassOitCmd;
            GFX::EnqueueCommand(bufferInOut, endRenderPassOitCmd);

            // Step2: Composition pass
            // Don't clear depth & colours and do not write to the depth buffer
            GFX::BeginRenderPassCommand beginRenderPassCompCmd;
            beginRenderPassCompCmd._name = Util::StringFormat("DO_OIT_PASS_2_%s", (i == 0 ? "HIGH" : "LOW")).c_str();
            beginRenderPassCompCmd._target = RenderTargetID(RenderTargetUsage::SCREEN);
            beginRenderPassCompCmd._descriptor.disableState(RTDrawDescriptor::State::CLEAR_DEPTH_BUFFER);
            beginRenderPassCompCmd._descriptor.disableState(RTDrawDescriptor::State::CLEAR_COLOUR_BUFFERS);
            beginRenderPassCompCmd._descriptor.drawMask().setEnabled(RTAttachmentType::Depth, 0, false);
            {
                RTBlendState& state0 = beginRenderPassCompCmd._descriptor.blendState(to_U8(GFXDevice::ScreenTargets::ALBEDO));
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
            descriptorSetCmd._set = _context.newDescriptorSet();
            descriptorSetCmd._set->_textureData.addTexture(accum, to_base(ShaderProgram::TextureUsage::UNIT0));
            descriptorSetCmd._set->_textureData.addTexture(revealage, to_base(ShaderProgram::TextureUsage::UNIT1));
            GFX::EnqueueCommand(bufferInOut, descriptorSetCmd);

            GFX::DrawCommand drawCmd;
            GenericDrawCommand drawCommand;
            drawCommand._primitiveType = PrimitiveType::TRIANGLES;
            drawCommand._drawCount = 1;
            drawCmd._drawCommands.push_back(drawCommand);
            GFX::EnqueueCommand(bufferInOut, drawCmd);

            GFX::EndRenderPassCommand endRenderPassCompCmd;
            GFX::EnqueueCommand(bufferInOut, endRenderPassCompCmd);
        }
    }
}

void RenderPassManager::doCustomPass(PassParams& params, GFX::CommandBuffer& bufferInOut) {
    // Tell the Rendering API to draw from our desired PoV
    GFX::SetCameraCommand setCameraCommand;
    setCameraCommand._camera = params._camera;
    GFX::EnqueueCommand(bufferInOut, setCameraCommand);

    GFX::SetClipPlanesCommand setClipPlanesCommand;
    setClipPlanesCommand._clippingPlanes = params._clippingPlanes;
    GFX::EnqueueCommand(bufferInOut, setClipPlanesCommand);

    RenderTarget& target = _context.renderTargetPool().renderTarget(params._target);
    prePass(params, target, bufferInOut);

    if (params._occlusionCull) {
        _context.constructHIZ(params._target, bufferInOut);
        _context.occlusionCull(getBufferData(params._stage, params._pass), target.getAttachment(RTAttachmentType::Depth, 0).texture(), bufferInOut);
    }

    mainPass(params, target, bufferInOut);
    woitPass(params, target, bufferInOut);
}

};