#include "Headers/RenderPassManager.h"
#include "Rendering/RenderPass/Headers/RenderPass.h"
#include "Rendering/RenderPass/Headers/RenderQueue.h"

namespace Divide {

RenderPassItem::RenderPassItem(const stringImpl& renderPassName, U8 sortKey)
    : _sortKey(sortKey) {
    _renderPass = std::make_unique<RenderPass>(renderPassName);
}

RenderPassItem::RenderPassItem(RenderPassItem&& other)
    : _renderPass(std::move(other._renderPass)) {}

RenderPassItem& RenderPassItem::operator=(RenderPassItem&& other) {
    _renderPass = std::move(other._renderPass);
    return *this;
}

RenderPassItem::~RenderPassItem() {}

RenderPassManager::RenderPassManager()
    : _renderPassesLocked(false), _renderPassesResetQueued(false) {
    RenderQueue::createInstance();
}

RenderPassManager::~RenderPassManager() {
    _renderPasses.clear();
    RenderQueue::destroyInstance();
}

void RenderPassManager::lock() {
    _renderPassesLocked = true;
    RenderQueue::getInstance().lock();
}

void RenderPassManager::unlock(bool resetNodes) {
    _renderPassesLocked = false;
    _renderPassesResetQueued = true;
    RenderQueue::getInstance().unlock();
}

void RenderPassManager::render(const SceneRenderState& sceneRenderState,
                               const SceneGraph& activeSceneGraph) {
    for (RenderPassItem& rpi : _renderPasses) {
        rpi.renderPass().render(sceneRenderState, activeSceneGraph);
    }
}

void RenderPassManager::addRenderPass(const stringImpl& renderPassName,
                                      U8 orderKey) {
    assert(!renderPassName.empty());

    _renderPasses.push_back(RenderPassItem(renderPassName, orderKey));
    std::sort(std::begin(_renderPasses), std::end(_renderPasses),
              [](const RenderPassItem& a, const RenderPassItem& b)
                  -> bool { return a.sortKey() < b.sortKey(); });
}

void RenderPassManager::removeRenderPass(const stringImpl& name) {
    for (std::vector<RenderPassItem>::iterator it = std::begin(_renderPasses);
         it != std::end(_renderPasses); ++it) {
        if (it->renderPass().getName().compare(name) == 0) {
            _renderPasses.erase(it);
            break;
        }
    }
}

U16 RenderPassManager::getLastTotalBinSize(U8 renderPassID) const {
    if (renderPassID < _renderPasses.size()) {
        return _renderPasses[renderPassID].renderPass().getLasTotalBinSize();
    }

    return 0;
}
};