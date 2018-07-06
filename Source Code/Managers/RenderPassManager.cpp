#include "Headers/RenderPassManager.h"

#include "Core/Headers/TaskPool.h"
#include "Managers/Headers/SceneManager.h"
#include "Rendering/Camera/Headers/Camera.h"
#include "Rendering/RenderPass/Headers/RenderQueue.h"

namespace Divide {

namespace {
    RTDrawDescriptor _noDepthClear;
    RTDrawDescriptor _depthOnly;
};

RenderPassManager::RenderPassManager()
    : _renderQueue(MemoryManager_NEW RenderQueue())
{
    _noDepthClear._clearDepthBufferOnBind = false;
    _noDepthClear._clearColourBuffersOnBind = true;
    _noDepthClear._drawMask.enableAll();

    _depthOnly._clearColourBuffersOnBind = true;
    _depthOnly._clearDepthBufferOnBind = true;
    _depthOnly._drawMask.disableAll();
    _depthOnly._drawMask.enabled(RTAttachment::Type::Depth, 0);
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
    // step1: cull nodes for current camera and pass
    SceneManager& mgr = SceneManager::instance();

    if (params.reflectionPlane) {
        params.camera->renderLookAtReflected(*params.reflectionPlane);
    } else {
        params.camera->renderLookAt();
    }

    GFXDevice& GFX = GFX_DEVICE;

    GFX.setRenderStage(params.stage);

    if (params.doPrePass) {
        GFX.setPrePass(true);
        
        Attorney::SceneManagerRenderPass::populateRenderQueue(mgr,
                                                              params.stage,
                                                              true,
                                                              params.pass);
        if (params.target) {
            params.target->begin(_depthOnly);
        }
            GFX.flushRenderQueues();
            Attorney::SceneManagerRenderPass::postRender(mgr, params.stage);

        if (params.target) {
            params.target->end();
        }
        
        
        GFX.constructHIZ(*params.target);

        if (params.occlusionCull) {
            RenderPassManager& passMgr = RenderPassManager::instance();
            RenderPass::BufferData& bufferData = passMgr.getBufferData(params.stage, params.pass);
            GFX.occlusionCull(bufferData, params.target->getAttachment(RTAttachment::Type::Depth, 0).asTexture());
        }
    }

    GFX.setPrePass(false);
    // step3: do renderer pass 1: light cull for Forward+ / G-buffer creation for Deferred
    GFX.setRenderStage(params.stage);

    if (params.stage != RenderStage::SHADOW) {
        Attorney::SceneManagerRenderPass::preRender(mgr, *params.target);
        if (!Config::DEBUG_HIZ_CULLING) {
            GFX.toggleDepthWrites(false);
        }
    }

    Attorney::SceneManagerRenderPass::populateRenderQueue(mgr,
                                                          params.stage,
                                                          !params.doPrePass,
                                                          params.pass);

    RTDrawDescriptor* drawPolicy = params.drawPolicy ? params.drawPolicy
                                                     : &(!Config::DEBUG_HIZ_CULLING ? _noDepthClear
                                                                                    : RenderTarget::defaultPolicy());
    if (params.target) {
        params.target->begin(*drawPolicy);
    }
        GFX.flushRenderQueues();
        Attorney::SceneManagerRenderPass::postRender(mgr, params.stage);

    if (params.target) {
        params.target->end();
    }
    if (params.stage != RenderStage::SHADOW) {
        if (!Config::DEBUG_HIZ_CULLING) {
            GFX.toggleDepthWrites(true);
        }
    }
}

};