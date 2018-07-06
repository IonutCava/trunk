#include "stdafx.h"

#include "Headers/RenderPassManager.h"

#include "Core/Headers/TaskPool.h"
#include "Core/Headers/StringHelper.h"
#include "Managers/Headers/SceneManager.h"
#include "Rendering/Camera/Headers/Camera.h"
#include "Rendering/RenderPass/Headers/RenderQueue.h"
#include "Platform/Video/Textures/Headers/Texture.h"

namespace Divide {

RenderPassManager::RenderPassManager(Kernel& parent, GFXDevice& context)
    : KernelComponent(parent),
      _context(context),
      _renderQueue(nullptr),
      _OITCompositionShader(nullptr)
{
}

RenderPassManager::~RenderPassManager()
{
    destroy();
}

bool RenderPassManager::init() {
    if (_renderQueue == nullptr) {
        _renderQueue = MemoryManager_NEW RenderQueue(_context);

        ResourceDescriptor shaderDesc("OITComposition");
        _OITCompositionShader = CreateResource<ShaderProgram>(parent().resourceCache(), shaderDesc);
        return true;
    }

    return false;
}

void RenderPassManager::destroy() {
    MemoryManager::DELETE_VECTOR(_renderPasses);
    MemoryManager::DELETE(_renderQueue);
    _OITCompositionShader.reset();
}

void RenderPassManager::render(SceneRenderState& sceneRenderState) {
    for (RenderPass* rp : _renderPasses) {
        rp->render(sceneRenderState);
    }
}

RenderPass& RenderPassManager::addRenderPass(const stringImpl& renderPassName,
                                             U8 orderKey,
                                             RenderStage renderStage) {
    assert(!renderPassName.empty());

    _renderPasses.emplace_back(MemoryManager_NEW RenderPass(*this, _context, renderPassName, orderKey, renderStage));
    RenderPass& item = *_renderPasses.back();

    std::sort(std::begin(_renderPasses), std::end(_renderPasses),
              [](RenderPass* a, RenderPass* b)
                  -> bool { return a->sortKey() < b->sortKey(); });

    assert(item.sortKey() == orderKey);

    return item;
}

void RenderPassManager::removeRenderPass(const stringImpl& name) {
    for (vectorImpl<RenderPass*>::iterator it = std::begin(_renderPasses);
         it != std::end(_renderPasses); ++it) {
        if ((*it)->getName().compare(name) == 0) {
            _renderPasses.erase(it);
            break;
        }
    }
}

U16 RenderPassManager::getLastTotalBinSize(RenderStage renderStage) const {
    return getPassForStage(renderStage)->getLastTotalBinSize();
}

RenderPass* RenderPassManager::getPassForStage(RenderStage renderStage) const {
    for (RenderPass* pass : _renderPasses) {
        if (pass->stageFlag() == renderStage) {
            return pass;
        }
    }

    DIVIDE_UNEXPECTED_CALL();
    return nullptr;
}

const RenderPass::BufferData& 
RenderPassManager::getBufferData(RenderStage renderStage, I32 bufferIndex) const {
    return getPassForStage(renderStage)->getBufferData(bufferIndex);
}

RenderPass::BufferData&
RenderPassManager::getBufferData(RenderStage renderStage, I32 bufferIndex) {
    return getPassForStage(renderStage)->getBufferData(bufferIndex);
}

void cleanCommandBuffer(CommandBuffer& cmdBuffer) {
    cmdBuffer.erase(std::remove_if(std::begin(cmdBuffer),
                                   std::end(cmdBuffer),
                                   [](const RenderPassCmd& passCmd) -> bool {
                                       return passCmd._subPassCmds.empty();
                                   }),
                    std::end(cmdBuffer));

    for (RenderPassCmd& pass : cmdBuffer) {
        RenderSubPassCmds& subPasses = pass._subPassCmds;
        for (RenderSubPassCmd& subPass : subPasses) {
            subPass._commands.clean();
        }

        subPasses.erase(std::remove_if(std::begin(subPasses),
                                       std::end(subPasses),
                                       [](const RenderSubPassCmd& subPassCmd) -> bool {
                                           return subPassCmd._commands.empty();
                                       }),
                        std::end(subPasses));

    }
}


RenderPassCmd RenderPassManager::prePass(const PassParams& params, const RenderTarget& target) {
    static const vectorImpl<RenderBinType> depthExclusionList
    {
        RenderBinType::RBT_DECAL,
        RenderBinType::RBT_TRANSLUCENT
    };

    GFX::ScopedDebugMessage(_context, Util::StringFormat("Custom pass ( %s ): PrePass", TypeUtil::renderStageToString(params.stage)), 0);
    RenderPassCmd cmd;

    // PrePass requires a depth buffer
    bool doPrePass = params.doPrePass && target.getAttachment(RTAttachmentType::Depth, 0).used();

    if (doPrePass) {
        SceneManager& sceneManager = parent().sceneManager();

        _context.setRenderStagePass(RenderStagePass(params.stage, RenderPassType::DEPTH_PASS));

        Attorney::SceneManagerRenderPass::populateRenderQueue(sceneManager,
                                                              *params.camera,
                                                              true,
                                                              params.pass);

        if (params.target._usage != RenderTargetUsage::COUNT) {
            cmd._renderTargetDescriptor = RenderTarget::defaultPolicyDepthOnly();
            if (params.bindTargets) {
                cmd._renderTarget = params.target;
            }
            U32 binCount = to_U32(RenderBinType::COUNT);
            RenderSubPassCmds binCmds(binCount);

            for (U32 i = 0; i < binCount; ++i) {
                if (std::find(std::begin(depthExclusionList),
                    std::end(depthExclusionList),
                    RenderBinType::_from_integral(i)) == std::cend(depthExclusionList))
                {
                    if (i != to_U32(RenderBinType::RBT_TRANSLUCENT)) {
                        _context.renderQueueToSubPasses(RenderBinType::_from_integral(i), binCmds[i]);
                    }
                }
            }

            cmd._subPassCmds.insert(std::cend(cmd._subPassCmds), std::cbegin(binCmds), std::cend(binCmds));

            RenderSubPassCmds postRenderSubPasses(1);
            Attorney::SceneManagerRenderPass::postRender(sceneManager, *params.camera, postRenderSubPasses);
            cmd._subPassCmds.insert(std::cend(cmd._subPassCmds), std::cbegin(postRenderSubPasses), std::cend(postRenderSubPasses));
        }
    }

    return cmd;
}

RenderPassCmd RenderPassManager::mainPass(const PassParams& params, RenderTarget& target) {
    static const vectorImpl<RenderBinType> shadowExclusionList
    {
        RenderBinType::RBT_DECAL,
        RenderBinType::RBT_SKY,
        RenderBinType::RBT_TRANSLUCENT
    };
    constexpr U32 binCount = to_U32(RenderBinType::COUNT);


    GFX::ScopedDebugMessage(_context, Util::StringFormat("Custom pass ( %s ): RenderPass", TypeUtil::renderStageToString(params.stage)), 1);

    RenderPassCmd cmd;
    SceneManager& sceneManager = parent().sceneManager();

    _context.setRenderStagePass(RenderStagePass(params.stage, RenderPassType::COLOUR_PASS));
    Attorney::SceneManagerRenderPass::populateRenderQueue(sceneManager,
                                                          *params.camera,
                                                          !params.doPrePass,
                                                          params.pass);

    if (params.target._usage != RenderTargetUsage::COUNT) {
        bool drawToDepth = true;
        if (params.stage != RenderStage::SHADOW) {
            Attorney::SceneManagerRenderPass::preRender(sceneManager, *params.camera, target);
            if (params.doPrePass) {
                drawToDepth = Config::DEBUG_HIZ_CULLING;
            }
        }

        if (params.stage == RenderStage::DISPLAY) {
            // Bind the depth buffers
            const Texture_ptr& depthBufferTexture = target.getAttachment(RTAttachmentType::Depth, 0).texture();
            depthBufferTexture->bind(to_U8(ShaderProgram::TextureUsage::DEPTH));

            const RTAttachment& velocityAttachment = target.getAttachment(RTAttachmentType::Colour, to_U8(GFXDevice::ScreenTargets::VELOCITY));
            if (velocityAttachment.used()) {
                const Texture_ptr& prevDepthTexture = _context.getPrevDepthBuffer();
                (prevDepthTexture ? prevDepthTexture : depthBufferTexture)->bind(to_U8(ShaderProgram::TextureUsage::DEPTH_PREV));
            }
        }

        RTDrawDescriptor& drawPolicy = 
            params.drawPolicy ? *params.drawPolicy
                              : (!Config::DEBUG_HIZ_CULLING ? RenderTarget::defaultPolicyKeepDepth()
                                                            : RenderTarget::defaultPolicy());

        drawPolicy.drawMask().setEnabled(RTAttachmentType::Depth, 0, drawToDepth);

        if (params.bindTargets) {
            cmd._renderTarget = params.target;
        }

        cmd._renderTargetDescriptor = drawPolicy;

        RenderSubPassCmds binCmds(binCount);

        if (params.stage == RenderStage::SHADOW) {
            for (U32 i = 0; i < binCount; ++i) {
                if (std::find(std::begin(shadowExclusionList),
                              std::end(shadowExclusionList),
                               RenderBinType::_from_integral(i)) == std::cend(shadowExclusionList))
                {
                    if (i != to_U32(RenderBinType::RBT_TRANSLUCENT)) {
                        _context.renderQueueToSubPasses(RenderBinType::_from_integral(i), binCmds[i]);
                    }
                }
            }
        } else {
            for (U32 i = 0; i < binCount; ++i) {
                if (i != to_U32(RenderBinType::RBT_TRANSLUCENT)) {
                    _context.renderQueueToSubPasses(RenderBinType::_from_integral(i), binCmds[i]);
                }
            }

        }

        cmd._subPassCmds.insert(std::cend(cmd._subPassCmds), std::cbegin(binCmds), std::cend(binCmds));

        RenderSubPassCmds postRenderSubPasses(1);
        Attorney::SceneManagerRenderPass::postRender(sceneManager, *params.camera, postRenderSubPasses);
        cmd._subPassCmds.insert(std::cend(cmd._subPassCmds), std::cbegin(postRenderSubPasses), std::cend(postRenderSubPasses));

        if (params.stage == RenderStage::DISPLAY) {
            /// These should be OIT rendered as well since things like debug nav meshes have translucency
            RenderSubPassCmds debugDrawSubPasses(1);
            Attorney::SceneManagerRenderPass::debugDraw(sceneManager, *params.camera, debugDrawSubPasses);
            cmd._subPassCmds.insert(std::cend(cmd._subPassCmds), std::cbegin(debugDrawSubPasses), std::cend(debugDrawSubPasses));
        }
    }

    return cmd;
}

std::pair<RenderPassCmd /*Accumulation*/, RenderPassCmd/*Composition*/>
RenderPassManager::woitPass(const PassParams& params, const RenderTarget& target) {
    static bool init = false;
    static RenderPassCmd compCmd;
    static RTDrawDescriptor noClearPolicy;
    static RenderSubPassCmd compSubPass;
    static RenderPassCmd oitCmd;

    // Weighted Blended Order Independent Transparency
    if (_context.renderQueueSize(RenderBinType::RBT_TRANSLUCENT) > 0) {

        if (!init) {
            oitCmd._renderTarget = RenderTargetID(RenderTargetUsage::OIT);
            RTBlendState& state0 = oitCmd._renderTargetDescriptor.blendState(0);
            RTBlendState& state1 = oitCmd._renderTargetDescriptor.blendState(1);

            state0._blendEnable = true;
            state0._blendProperties._blendSrc = BlendProperty::ONE;
            state0._blendProperties._blendDest = BlendProperty::ONE;
            state0._blendProperties._blendOp = BlendOperation::ADD;

            state1._blendEnable = true;
            state1._blendProperties._blendSrc = BlendProperty::ZERO;
            state1._blendProperties._blendDest = BlendProperty::INV_SRC_COLOR;
            state1._blendProperties._blendOp = BlendOperation::ADD;

            // Don't clear depth & colours and do not write to the depth buffer
            noClearPolicy.disableState(RTDrawDescriptor::State::CLEAR_DEPTH_BUFFER);
            noClearPolicy.disableState(RTDrawDescriptor::State::CLEAR_COLOUR_BUFFERS);
            noClearPolicy.drawMask().setEnabled(RTAttachmentType::Depth, 0, false);
            noClearPolicy.blendState(0)._blendEnable = true;
            noClearPolicy.blendState(0)._blendProperties._blendSrc = BlendProperty::SRC_ALPHA;
            noClearPolicy.blendState(0)._blendProperties._blendDest = BlendProperty::INV_SRC_ALPHA;

            compCmd._renderTarget = RenderTargetID(RenderTargetUsage::SCREEN);
            compCmd._renderTargetDescriptor = noClearPolicy;
            PipelineDescriptor pipelineDescriptor;
            pipelineDescriptor._stateHash = _context.getDefaultStateBlock(true);
            pipelineDescriptor._shaderProgram = _OITCompositionShader;

            BindPipelineCommand bindPipelineCmd;
            bindPipelineCmd._pipeline = _context.newPipeline(pipelineDescriptor);
            compSubPass._commands.add(bindPipelineCmd);

            GenericDrawCommand drawCommand;
            drawCommand.primitiveType(PrimitiveType::TRIANGLES);
            drawCommand.drawCount(1);
            DrawCommand drawCmd;
            drawCmd._drawCommands.push_back(drawCommand);
            compSubPass._commands.add(drawCmd);

            init = true;
        } else {
            oitCmd._subPassCmds.resize(0);
            compCmd._subPassCmds.resize(0);
        }

        // Step1: Draw translucent items into the accumulation and revealage buffers
        RenderSubPassCmd subPass;
        _context.renderQueueToSubPasses(RenderBinType::RBT_TRANSLUCENT, subPass);
        oitCmd._subPassCmds.insert(std::cend(oitCmd._subPassCmds), subPass);

        // Step2: Composition pass
        TextureData accum = _context.renderTargetPool().renderTarget(oitCmd._renderTarget).getAttachment(RTAttachmentType::Colour, to_U8(GFXDevice::ScreenTargets::ACCUMULATION)).texture()->getData();
        TextureData revealage = _context.renderTargetPool().renderTarget(oitCmd._renderTarget).getAttachment(RTAttachmentType::Colour, to_U8(GFXDevice::ScreenTargets::REVEALAGE)).texture()->getData();
        accum.setHandleLow(0);
        revealage.setHandleLow(1);
        compSubPass._textures.clear();
        compSubPass._textures.addTexture(accum);
        compSubPass._textures.addTexture(revealage);

        compCmd._subPassCmds.push_back(compSubPass);
    }

    return std::make_pair(oitCmd, compCmd);
}

void RenderPassManager::doCustomPass(PassParams& params) {
    static CommandBuffer commandBuffer;

    commandBuffer.resize(0);

    // Tell the Rendering API to draw from our desired PoV
    _context.renderFromCamera(*params.camera);
    _context.setClipPlanes(params.clippingPlanes);

    RenderTarget& target = _context.renderTargetPool().renderTarget(params.target);

    commandBuffer.push_back(prePass(params, target));
    cleanCommandBuffer(commandBuffer);

    if (!commandBuffer.empty()) {
        _context.flushCommandBuffer(commandBuffer);
        if (params.occlusionCull) {
            _context.constructHIZ(params.target);
            _context.occlusionCull(getBufferData(params.stage, params.pass), target.getAttachment(RTAttachmentType::Depth, 0).texture());
        }
    }

    commandBuffer.push_back(mainPass(params, target));
    std::pair<RenderPassCmd, RenderPassCmd> woit = woitPass(params, target);
    //commandBuffer.push_back(woit.first);
    //commandBuffer.push_back(woit.second);
    cleanCommandBuffer(commandBuffer);
    _context.flushCommandBuffer(commandBuffer);
}

};