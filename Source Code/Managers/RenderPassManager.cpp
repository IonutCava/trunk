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
      _renderQueue(nullptr)
{
}

RenderPassManager::~RenderPassManager()
{
    destroy();
}

bool RenderPassManager::init() {
    if (_renderQueue == nullptr) {
        _renderQueue = MemoryManager_NEW RenderQueue(_context);
        return true;
    }

    return false;
}

void RenderPassManager::destroy() {
    MemoryManager::DELETE_VECTOR(_renderPasses);
    MemoryManager::DELETE(_renderQueue);
}

void RenderPassManager::render(SceneRenderState& sceneRenderState) {
    // Attempt to build draw commands in parallel


    TaskPool& pool = Application::instance().kernel().taskPool();

    U8 passCount = to_ubyte(_renderPasses.size());
    _renderCmdTasks.clear();
    _renderCmdTasks.reserve(passCount);

    for (U8 i = 0; i < passCount; ++i)
    {
        RenderPass* rp  =  _renderPasses[i];
        _renderCmdTasks.emplace_back(CreateTask(pool,
            [rp](const Task& parentTask) mutable
            {
                rp->generateDrawCommands();
            }));
        _renderCmdTasks[i].startTask(Task::TaskPriority::HIGH);
    }

    for (U8 i = 0; i < passCount; ++i) {
        RenderPass* rp = _renderPasses[i];
        _renderCmdTasks[i].wait();
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
            GenericDrawCommands& drawCommands = subPass._commands;
            drawCommands.erase(std::remove_if(std::begin(drawCommands),
                                              std::end(drawCommands),
                                              [](const GenericDrawCommand& cmd) -> bool {
                                                  return cmd.drawCount() == 0;
                                              }),
                              std::end(drawCommands));
        }

        subPasses.erase(std::remove_if(std::begin(subPasses),
                                       std::end(subPasses),
                                       [](const RenderSubPassCmd& subPassCmd) -> bool {
                                           return subPassCmd._commands.empty();
                                       }),
                        std::end(subPasses));

    }
}

void RenderPassManager::doCustomPass(PassParams& params) {
    static CommandBuffer commandBuffer;
    commandBuffer.resize(0);


    // step1: cull nodes for current camera and pass
    SceneManager& mgr = parent().sceneManager();

    // Tell the Rendering API to draw from our desired PoV
    _context.renderFromCamera(*params.camera);
    _context.setClipPlanes(params.clippingPlanes);
    
    if (params.doPrePass) {
        RenderTarget& target = _context.renderTarget(params.target);
        if (!target.getAttachment(RTAttachment::Type::Depth, 0).used()) {
            params.doPrePass = false;
        }
    }

    if (params.doPrePass) {
        _context.setRenderStagePass(RenderStagePass(params.stage, RenderPassType::DEPTH_PASS));

        GFX::ScopedDebugMessage(_context, Util::StringFormat("Custom pass ( %s ): PrePass", TypeUtil::renderStageToString(params.stage)).c_str(), 0);

        Attorney::SceneManagerRenderPass::populateRenderQueue(mgr,
                                                              *params.camera,
                                                              true,
                                                              params.pass);

        if (params.target._usage != RenderTargetUsage::COUNT) {
            RenderTarget& target = _context.renderTarget(params.target);
            const Texture_ptr& depthBufferTexture = target.getAttachment(RTAttachment::Type::Depth, 0).asTexture();

            RenderPassCmd cmd;
            cmd._renderTarget = params.target;
            cmd._renderTargetDescriptor = RenderTarget::defaultPolicyDepthOnly();
            _context.renderQueueToSubPasses(cmd);
            RenderSubPassCmds postRenderSubPasses(1);
            Attorney::SceneManagerRenderPass::postRender(mgr, *params.camera, postRenderSubPasses);
            cmd._subPassCmds.insert(std::cend(cmd._subPassCmds), std::cbegin(postRenderSubPasses), std::cend(postRenderSubPasses));
            commandBuffer.push_back(cmd);
            cleanCommandBuffer(commandBuffer);
            _context.flushCommandBuffer(commandBuffer);
            commandBuffer.resize(0);

            if (params.occlusionCull) {
                _context.constructHIZ(target);
                const RenderPass::BufferData& bufferData = getBufferData(params.stage, params.pass);
                _context.occlusionCull(bufferData, depthBufferTexture);
            }
        }
    }

    _context.setRenderStagePass(RenderStagePass(params.stage, RenderPassType::COLOUR_PASS));
    GFX::ScopedDebugMessage(_context, Util::StringFormat("Custom pass ( %s ): RenderPass", TypeUtil::renderStageToString(params.stage)).c_str(), 1);

    // step3: do renderer pass 1: light cull for Forward+ / G-buffer creation for Deferred
    Attorney::SceneManagerRenderPass::populateRenderQueue(mgr,
                                                          *params.camera,
                                                          !params.doPrePass,
                                                          params.pass);
    if (params.target._usage != RenderTargetUsage::COUNT) {
        bool drawToDepth = true;
        if (params.stage != RenderStage::SHADOW) {
            Attorney::SceneManagerRenderPass::preRender(mgr, *params.camera, _context.renderTarget(params.target));
            if (params.doPrePass) {
                drawToDepth = Config::DEBUG_HIZ_CULLING;
            }
        }

        if (params.stage == RenderStage::DISPLAY) {
            // Bind the depth buffers
            RenderTarget& target = _context.renderTarget(params.target);
            const Texture_ptr& depthBufferTexture = target.getAttachment(RTAttachment::Type::Depth, 0).asTexture();
            depthBufferTexture->bind(to_const_ubyte(ShaderProgram::TextureUsage::DEPTH), 0);

            const RTAttachment& velocityAttachment = target.getAttachment(RTAttachment::Type::Colour,
                                                                          to_const_ubyte(GFXDevice::ScreenTargets::VELOCITY));
            if (velocityAttachment.used()) {
                const Texture_ptr& prevDepthTexture = _context.getPrevDepthBuffer();
                (prevDepthTexture ? prevDepthTexture : depthBufferTexture)->bind(to_const_ubyte(ShaderProgram::TextureUsage::DEPTH_PREV), 0);
            }
        }

        RTDrawDescriptor& drawPolicy = params.drawPolicy ? *params.drawPolicy
                                                         : (!Config::DEBUG_HIZ_CULLING ? RenderTarget::defaultPolicyKeepDepth()
                                                                                       : RenderTarget::defaultPolicy());
        drawPolicy.drawMask().setEnabled(RTAttachment::Type::Depth,  0, drawToDepth);

        RenderPassCmd cmd;
        cmd._renderTarget = params.target;
        cmd._renderTargetDescriptor = drawPolicy;
        _context.renderQueueToSubPasses(cmd);

        RenderSubPassCmds postRenderSubPasses(1);
        Attorney::SceneManagerRenderPass::postRender(mgr, *params.camera, postRenderSubPasses);
        cmd._subPassCmds.insert(std::cend(cmd._subPassCmds), std::cbegin(postRenderSubPasses), std::cend(postRenderSubPasses));

        commandBuffer.push_back(cmd);

        cleanCommandBuffer(commandBuffer);
        _context.flushCommandBuffer(commandBuffer);
        commandBuffer.resize(0);
    }
}

};