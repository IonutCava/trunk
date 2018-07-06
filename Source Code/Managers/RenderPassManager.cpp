#include "Headers/RenderPassManager.h"

#include "Core/Headers/TaskPool.h"
#include "Managers/Headers/SceneManager.h"
#include "Rendering/Camera/Headers/Camera.h"
#include "Rendering/RenderPass/Headers/RenderQueue.h"

namespace Divide {

RenderPassManager::RenderPassManager()
    : _renderQueue(MemoryManager_NEW RenderQueue())
{
}

RenderPassManager::~RenderPassManager()
{
    for (RenderPass* rp : _renderPasses) {
        MemoryManager::DELETE(rp);
    }

    MemoryManager::DELETE(_renderQueue);
}

void RenderPassManager::render(SceneRenderState& sceneRenderState) {
    // Attempt to build draw commands in parallel
    TaskHandle renderCommandTask = CreateTask(DELEGATE_CBK_PARAM<bool>());
    for (RenderPass* rp : _renderPasses)
    {
        renderCommandTask.addChildTask(CreateTask(
            [rp](const std::atomic_bool& stopRequested) mutable
            {
                rp->generateDrawCommands();
            })._task
            )->startTask(Task::TaskPriority::HIGH);
    }
    
    renderCommandTask.startTask(Task::TaskPriority::HIGH);
    renderCommandTask.wait();

    for (RenderPass* rp : _renderPasses) {
        rp->render(sceneRenderState);
    }
}

RenderPass& RenderPassManager::addRenderPass(const stringImpl& renderPassName,
                                             U8 orderKey,
                                             RenderStage renderStage) {
    assert(!renderPassName.empty());

    _renderPasses.emplace_back(MemoryManager_NEW RenderPass(renderPassName, orderKey, renderStage));
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
    SceneManager& mgr = SceneManager::instance();

    GFXDevice& GFX = GFX_DEVICE;

    // Tell the Rendering API to draw from our desired PoV
    GFX.renderFromCamera(*params.camera);
   
    GFX.setRenderStage(params.stage);

    if (params.doPrePass) {
        GFX.setPrePass(true);
        
        Attorney::SceneManagerRenderPass::populateRenderQueue(mgr,
                                                              params.stage,
                                                              true,
                                                              params.pass);

        if (params.target._usage != RenderTargetUsage::COUNT) {
            RenderPassCmd cmd;
            cmd._renderTarget = params.target;
            cmd._renderTargetDescriptor = RenderTarget::defaultPolicyDepthOnly();
            GFX.renderQueueToSubPasses(cmd);
            RenderSubPassCmds postRenderSubPasses(1);
            Attorney::SceneManagerRenderPass::postRender(mgr, *params.camera, params.stage, postRenderSubPasses);
            cmd._subPassCmds.insert(std::cend(cmd._subPassCmds), std::cbegin(postRenderSubPasses), std::cend(postRenderSubPasses));
            commandBuffer.push_back(cmd);
            cleanCommandBuffer(commandBuffer);
            GFX.flushCommandBuffer(commandBuffer);
            commandBuffer.resize(0);

            RenderTarget& target = GFX.renderTarget(params.target);
            GFX.constructHIZ(target);

            if (params.occlusionCull) {
                RenderPassManager& passMgr = RenderPassManager::instance();
                RenderPass::BufferData& bufferData = passMgr.getBufferData(params.stage, params.pass);
                GFX.occlusionCull(bufferData, target.getAttachment(RTAttachment::Type::Depth, 0).asTexture());
            }
        }
    }

    GFX.setPrePass(false);
    // step3: do renderer pass 1: light cull for Forward+ / G-buffer creation for Deferred
    GFX.setRenderStage(params.stage);

    Attorney::SceneManagerRenderPass::populateRenderQueue(mgr,
                                                          params.stage,
                                                          !params.doPrePass,
                                                          params.pass);
    if (params.target._usage != RenderTargetUsage::COUNT) {
        bool drawToDepth = true;
        if (params.stage != RenderStage::SHADOW) {
            Attorney::SceneManagerRenderPass::preRender(mgr, *params.camera, GFX.renderTarget(params.target));
            if (params.doPrePass) {
                drawToDepth = Config::DEBUG_HIZ_CULLING;
            }
        }

        RTDrawDescriptor& drawPolicy = params.drawPolicy ? *params.drawPolicy
                                                         : (!Config::DEBUG_HIZ_CULLING ? RenderTarget::defaultPolicyKeepDepth()
                                                                                       : RenderTarget::defaultPolicy());
        drawPolicy.drawMask().setEnabled(RTAttachment::Type::Depth, 0, drawToDepth);

        RenderPassCmd cmd;
        cmd._renderTarget = params.target;
        cmd._renderTargetDescriptor = drawPolicy;
        GFX.renderQueueToSubPasses(cmd);
        RenderSubPassCmds postRenderSubPasses(1);
        Attorney::SceneManagerRenderPass::postRender(mgr, *params.camera, params.stage, postRenderSubPasses);
        cmd._subPassCmds.insert(std::cend(cmd._subPassCmds), std::cbegin(postRenderSubPasses), std::cend(postRenderSubPasses));
        commandBuffer.push_back(cmd);
        cleanCommandBuffer(commandBuffer);
        GFX.flushCommandBuffer(commandBuffer);
        commandBuffer.resize(0);
    }
}

};