#include "Headers/RenderPassManager.h"

#include "Core/Headers/TaskPool.h"
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
                                             std::initializer_list<RenderStage> renderStages) {
    assert(!renderPassName.empty());

    _renderPasses.emplace_back(MemoryManager_NEW RenderPass(renderPassName, orderKey, renderStages));
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
        if (pass->hasStageFlag(renderStage)) {
            return pass;
        }
    }

    DIVIDE_UNEXPECTED_CALL();
    return nullptr;
}

const RenderPass::BufferData& 
RenderPassManager::getBufferData(RenderStage renderStage, I32 pass, U32 stage) const {
    return getPassForStage(renderStage)->getBufferData(pass, stage);
}

RenderPass::BufferData&
RenderPassManager::getBufferData(RenderStage renderStage, I32 pass, U32 stage) {
    return getPassForStage(renderStage)->getBufferData(pass, stage);
}

};