#include "Headers/RenderPassManager.h"
#include "Rendering/RenderPass/Headers/RenderQueue.h"

namespace Divide {

RenderPassManager::RenderPassManager()
{
    _renderQueue = std::make_unique<RenderQueue>();
}

RenderPassManager::~RenderPassManager()
{
    _renderPasses.clear();
}

void RenderPassManager::render(SceneRenderState& sceneRenderState) {
    for (RenderPass& rp : _renderPasses) {
        rp.render(sceneRenderState);
    }
}

RenderPass& RenderPassManager::addRenderPass(const stringImpl& renderPassName,
                                             U8 orderKey,
                                             std::initializer_list<RenderStage> renderStages) {
    assert(!renderPassName.empty());

    _renderPasses.push_back(RenderPass(renderPassName, orderKey, renderStages));
    RenderPass& item = _renderPasses.back();

    std::sort(std::begin(_renderPasses), std::end(_renderPasses),
              [](const RenderPass& a, const RenderPass& b)
                  -> bool { return a.sortKey() < b.sortKey(); });

    assert(item.sortKey() == orderKey);

    return item;
}

void RenderPassManager::removeRenderPass(const stringImpl& name) {
    for (vectorImpl<RenderPass>::iterator it = std::begin(_renderPasses);
         it != std::end(_renderPasses); ++it) {
        if (it->getName().compare(name) == 0) {
            _renderPasses.erase(it);
            break;
        }
    }
}

U16 RenderPassManager::getLastTotalBinSize(RenderStage displayStage) const {
    for (const RenderPass& pass : _renderPasses) {
        if (pass.hasStageFlag(displayStage)) {
            return pass.getLastTotalBinSize();
        }
    }

    return 0;
}
};