#include "stdafx.h"

#include "Headers/RenderBin.h"

#include "ECS/Components/Headers/BoundsComponent.h"
#include "ECS/Components/Headers/RenderingComponent.h"
#include "Geometry/Material/Headers/Material.h"
#include "Graphs/Headers/SceneGraphNode.h"
#include "Managers/Headers/RenderPassManager.h"
#include "Utility/Headers/Localization.h"

namespace Divide {

namespace {
    constexpr U32 AVERAGE_BIN_SIZE = 127;
}

RenderBin::RenderBin(const RenderBinType rbType, const RenderStage stage) 
    : _rbType(rbType),
      _stage(stage)

{
    _renderBinStack.reserve(AVERAGE_BIN_SIZE);
}

const RenderBinItem& RenderBin::getItem(const U16 index) const {
    SharedLock<SharedMutex> r_lock(_renderBinLock);

    assert(index < _renderBinStack.size());
    return _renderBinStack[index];
}

void RenderBin::sort(const RenderingOrder renderOrder) {
    OPTICK_EVENT();

    switch (renderOrder) {
        case RenderingOrder::BY_STATE: {
            UniqueLock<SharedMutex> w_lock(_renderBinLock);
            // Sorting opaque items is a 3 step process:
            // 1: sort by shaders
            // 2: if the shader is identical, sort by state hash
            // 3: if shader is identical and state hash is identical, sort by albedo ID
            eastl::sort(begin(_renderBinStack),
                        end(_renderBinStack),   
                        [](const RenderBinItem& a, const RenderBinItem& b) -> bool {
                            // Sort by shader in all states The sort key is the shader id (for now)
                            if (a._shaderKey != b._shaderKey) return a._shaderKey < b._shaderKey;
                            // If the shader values are the same, we use the state hash for sorting
                            // The _stateHash is a CRC value created based on the RenderState.
                            if (a._stateHash != b._stateHash) return a._stateHash < b._stateHash;
                            // If both the shader are the same and the state hashes match,
                            // we sort by the secondary key (usually the texture id)
                            if (a._textureKey != b._textureKey) return a._textureKey < b._textureKey;
                            // ... and then finally fallback to front to back
                            return a._distanceToCameraSq < b._distanceToCameraSq;
                        });
        } break;
        case RenderingOrder::BACK_TO_FRONT: {
            UniqueLock<SharedMutex> w_lock(_renderBinLock);
            eastl::sort(begin(_renderBinStack),
                        end(_renderBinStack),
                        [](const RenderBinItem& a, const RenderBinItem& b) -> bool {
                            return a._distanceToCameraSq > b._distanceToCameraSq;
                        });
        } break;
        case RenderingOrder::FRONT_TO_BACK: {
            UniqueLock<SharedMutex> w_lock(_renderBinLock);
            eastl::sort(begin(_renderBinStack),
                        end(_renderBinStack),
                        [](const RenderBinItem& a, const RenderBinItem& b) -> bool {
                            return a._distanceToCameraSq < b._distanceToCameraSq;
                        });
        } break;
        case RenderingOrder::WATER_FIRST: {
            UniqueLock<SharedMutex> w_lock(_renderBinLock);
            eastl::sort(begin(_renderBinStack),
                        end(_renderBinStack),
                        [](const RenderBinItem& a, const RenderBinItem&) -> bool {
                            return a._renderable->getSGN()->getNode().type() == SceneNodeType::TYPE_WATER;
                        });
        } break;
        case RenderingOrder::NONE: {
            // no need to sort
        } break;
        case RenderingOrder::COUNT: {
            Console::errorfn(Locale::get(_ID("ERROR_INVALID_RENDER_BIN_SORT_ORDER")), _rbType._to_string());
        } break;
    }
}

U16 RenderBin::getSortedNodes(SortedQueue& nodes) const {
    OPTICK_EVENT();

    nodes.resize(0);
    nodes.reserve(getBinSize());
    {
        SharedLock<SharedMutex> r_lock(_renderBinLock);
        for (const RenderBinItem& item : _renderBinStack) {
            nodes.emplace_back(item._renderable);
        }
    }

    return to_U16(getBinSize());
}

void RenderBin::refresh() {
    UniqueLock<SharedMutex> w_lock(_renderBinLock);
    _renderBinStack.resize(0);
    _renderBinStack.reserve(AVERAGE_BIN_SIZE);
}

void RenderBin::addNodeToBin(const SceneGraphNode* sgn,
                             const RenderPackage& pkg,
                             const RenderStagePass& renderStagePass,
                             const F32 minDistToCameraSq,
                             const bool lock)
{
    RenderingComponent* const rComp = sgn->get<RenderingComponent>();

    RenderBinItem item = {};
    item._renderable = rComp;

    // Sort by state hash depending on the current rendering stage
    // Save the render state hash value for sorting
    item._stateHash = rComp->getSortKeyHash(renderStagePass);
    item._distanceToCameraSq = minDistToCameraSq;

    const Material_ptr& nodeMaterial = rComp->getMaterialInstance();
    if (nodeMaterial) {
        nodeMaterial->getSortKeys(renderStagePass, item._shaderKey, item._textureKey);
    }

    item._dataIndex = pkg.lastDataIndex();

    ScopedLock<SharedMutex> w_lock(_renderBinLock, lock, false);
    _renderBinStack.push_back(item);
}

void RenderBin::populateRenderQueue(const RenderStagePass stagePass, RenderQueuePackages& queueInOut) const {
    OPTICK_EVENT();

    for (const RenderBinItem& item : _renderBinStack) {
        queueInOut.push_back(&item._renderable->getDrawPackage(stagePass));
    }
}

void RenderBin::postRender(const SceneRenderState& renderState, const RenderStagePass stagePass, GFX::CommandBuffer& bufferInOut) {
    for (const RenderBinItem& item : _renderBinStack) {
        Attorney::RenderingCompRenderBin::postRender(item._renderable, renderState, stagePass, bufferInOut);
    }
}

U16 RenderBin::getBinSize() const {
    SharedLock<SharedMutex> r_lock(_renderBinLock);
    return to_U16(_renderBinStack.size());
}

} // namespace Divide