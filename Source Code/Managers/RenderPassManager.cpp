#include "Headers/RenderPassManager.h"
#include "Rendering/RenderPass/Headers/RenderQueue.h"

namespace Divide {

RenderPassItem::RenderPassItem(const stringImpl& renderPassName, U8 sortKey)
    : _sortKey(sortKey),
      _renderPass(renderPassName)
{
}

RenderPassItem::~RenderPassItem()
{
}

RenderPassManager::RenderPassManager()
{
    _renderQueue = std::make_unique<RenderQueue>();
}

RenderPassManager::~RenderPassManager()
{
    _renderPasses.clear();
}

void RenderPassManager::render(const SceneRenderState& sceneRenderState) {
    for (RenderPassItem& rpi : _renderPasses) {
        rpi.renderPass().render(sceneRenderState);
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
    for (vectorImpl<RenderPassItem>::iterator it = std::begin(_renderPasses);
         it != std::end(_renderPasses); ++it) {
        if (it->renderPass().getName().compare(name) == 0) {
            _renderPasses.erase(it);
            break;
        }
    }
}

U16 RenderPassManager::getLastTotalBinSize(U8 renderPassID) const {
    if (renderPassID < _renderPasses.size()) {
        return _renderPasses[renderPassID].renderPass().getLastTotalBinSize();
    }

    return 0;
}
};