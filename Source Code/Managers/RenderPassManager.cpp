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


void RenderPassManager::doCustomPass(PassParams& params) {
    static CommandBuffer commandBuffer;
    commandBuffer.resize(0);

    // step1: cull nodes for current camera and pass
    SceneManager& mgr = SceneManager::instance();

    GFXDevice& GFX = GFX_DEVICE;
    mat4<F32> viewMat; vec3<F32> eyeVec;
    params.camera->renderLookAt(viewMat, eyeVec);
    // Tell the Rendering API to draw from our desired PoV
    GFX.lookAt(viewMat, eyeVec);

    GFX.setRenderStage(params.stage);

    if (params.doPrePass) {
        GFX.setPrePass(true);
        
        Attorney::SceneManagerRenderPass::populateRenderQueue(mgr,
                                                              params.stage,
                                                              true,
                                                              params.pass);

        if (params.target._usage != RenderTargetUsage::COUNT) {
            RenderTarget& target = GFX.renderTarget(params.target);
            target.begin(RenderTarget::defaultPolicyDepthOnly());

            RenderPassCmd cmd;
            cmd._renderTarget = params.target;
            cmd._renderTargetDescriptor = *(params.drawPolicy); |
            GFX.renderQueueToSubPasses(cmd);
            commandBuffer.push_back(cmd);
            GFX.flushCommandBuffer(commandBuffer);
            commandBuffer.resize(0);


            Attorney::SceneManagerRenderPass::postRender(mgr, params.stage);

            target.end();
    
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
        RenderTarget& target = GFX.renderTarget(params.target);


        bool drawToDepth = true;
        if (params.stage != RenderStage::SHADOW) {
            Attorney::SceneManagerRenderPass::preRender(mgr, target);
            if (params.doPrePass && !Config::DEBUG_HIZ_CULLING) {
                drawToDepth = false;
            }
        }

        RTDrawDescriptor* drawPolicy = params.drawPolicy ? params.drawPolicy
                                                         : &(!Config::DEBUG_HIZ_CULLING ? RenderTarget::defaultPolicyKeepDepth()
                                                                                        : RenderTarget::defaultPolicy());
        drawPolicy->drawMask().setEnabled(RTAttachment::Type::Depth, 0, drawToDepth);

        target.begin(*drawPolicy);
    
        RenderPassCmd cmd;
        cmd._renderTarget = params.target;
        cmd._renderTargetDescriptor = *(params.drawPolicy);|
        GFX.renderQueueToSubPasses(cmd);
        commandBuffer.push_back(cmd);
        GFX.flushCommandBuffer(commandBuffer);
        commandBuffer.resize(0);

        Attorney::SceneManagerRenderPass::postRender(mgr, params.stage);

        target.end();
    }
}

};