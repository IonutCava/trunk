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
};

RenderBin::RenderBin(const RenderBinType rbType) : _rbType(rbType)
{
    for (RenderBinStack& stack : _renderBinStack) {
        stack.reserve(AVERAGE_BIN_SIZE);
    }
}

const RenderBinItem& RenderBin::getItem(const RenderStage stage, const U16 index) const {
    assert(index < _renderBinStack[to_base(stage)].size());
    return _renderBinStack[to_base(stage)][index];
}

void RenderBin::sort(const RenderStage stage, const RenderingOrder renderOrder) {
    OPTICK_EVENT();

    RenderBinStack& stack = _renderBinStack[to_U8(stage)];

    switch (renderOrder) {
        case RenderingOrder::BY_STATE: {
            // Sorting opaque items is a 3 step process:
            // 1: sort by shaders
            // 2: if the shader is identical, sort by state hash
            // 3: if shader is identical and state hash is identical, sort by albedo ID
            eastl::sort(stack.begin(),
                        stack.end(),   
                        [](const RenderBinItem& a, const RenderBinItem& b) -> bool {
                            // Sort by shader in all states The sort key is the shader id (for now)
                            if (a._sortKeyA != b._sortKeyA) {
                                return a._sortKeyA < b._sortKeyA;
                            }
                            // If the shader values are the same, we use the state hash for sorting
                            // The _stateHash is a CRC value created based on the RenderState.
                            if (a._stateHash != b._stateHash) {
                                return a._stateHash < b._stateHash;
                            }
                            // If both the shader are the same and the state hashes match,
                            // we sort by the secondary key (usually the texture id)
                            if (a._sortKeyB != b._sortKeyB) {
                                return a._sortKeyB < b._sortKeyB;
                            }
                            // If the data indices aren't consecutive, handle that first ...
                            /*if (a._dataIndex != 0u && a._dataIndex != b._dataIndex + 1) {
                                return a._dataIndex < b._dataIndex;
                            }*/
                            // ... and then finally fallback is front to back
                            return a._distanceToCameraSq < b._distanceToCameraSq;
                        });
        } break;
        case RenderingOrder::BACK_TO_FRONT: {
            eastl::sort(stack.begin(),
                        stack.end(),
                        [](const RenderBinItem& a, const RenderBinItem& b) -> bool {
                            return a._distanceToCameraSq > b._distanceToCameraSq;
                        });
        } break;
        case RenderingOrder::FRONT_TO_BACK: {
            eastl::sort(stack.begin(),
                        stack.end(),
                        [](const RenderBinItem& a, const RenderBinItem& b) -> bool {
                            return a._distanceToCameraSq < b._distanceToCameraSq;
                        });
        } break;
        case RenderingOrder::WATER_FIRST: {
            eastl::sort(stack.begin(),
                        stack.end(),
                        [](const RenderBinItem& a, const RenderBinItem& b) -> bool {
                            return a._renderable->getSGN()->getNode().type() == SceneNodeType::TYPE_WATER;
                        });
        } break;
        case RenderingOrder::NONE: {
            // no need to sort
        } break;
        default:
        case RenderingOrder::COUNT: {
            Console::errorfn(Locale::get(_ID("ERROR_INVALID_RENDER_BIN_SORT_ORDER")), _rbType._to_string());
        } break;
    };
}

U16 RenderBin::getSortedNodes(RenderStage stage, SortedQueue& nodes) const {
    OPTICK_EVENT();

    nodes.resize(0);
    nodes.reserve(getBinSize(stage));

    const RenderBinStack& stack = _renderBinStack[to_base(stage)];
    for (const RenderBinItem& item : stack) {
        nodes.emplace_back(item._renderable);
    }

    return to_U16(stack.size());
}

void RenderBin::refresh(RenderStage stage) {
    _renderBinStack[to_base(stage)].clear();
    _renderBinStack[to_base(stage)].reserve(AVERAGE_BIN_SIZE);
}

void RenderBin::addNodeToBin(const SceneGraphNode* sgn, const RenderPackage& pkg, const RenderStagePass& stagePass, const F32 minDistToCameraSq) {
    RenderingComponent* const rComp = sgn->get<RenderingComponent>();

    const U8 stageIndex = to_U8(stagePass._stage);

    RenderBinItem item = {};
    item._renderable = rComp;

    // Sort by state hash depending on the current rendering stage
    // Save the render state hash value for sorting
    item._stateHash = rComp->getSortKeyHash(stagePass);
    item._distanceToCameraSq = minDistToCameraSq;

    const Material_ptr& nodeMaterial = rComp->getMaterialInstance();
    if (nodeMaterial) {
        nodeMaterial->getSortKeys(stagePass, item._sortKeyA, item._sortKeyB);
    } else {
        item._sortKeyA = to_I64(_renderBinStack[stageIndex].size() + 1);
        item._sortKeyB = to_I32(item._sortKeyA);
    }

    item._dataIndex = pkg.lastDataIndex();

    _renderBinStack[stageIndex].push_back(item);
}

void RenderBin::populateRenderQueue(RenderStagePass stagePass, RenderQueuePackages& queueInOut) const {
    OPTICK_EVENT();

    for (const RenderBinItem& item : _renderBinStack[to_base(stagePass._stage)]) {
        queueInOut.push_back(&item._renderable->getDrawPackage(stagePass));
    }
}

void RenderBin::postRender(const SceneRenderState& renderState, RenderStagePass stagePass, GFX::CommandBuffer& bufferInOut) {
    for (const RenderBinItem& item : _renderBinStack[to_base(stagePass._stage)]) {
        Attorney::RenderingCompRenderBin::postRender(item._renderable, renderState, stagePass, bufferInOut);
    }
}

U16 RenderBin::getBinSize(RenderStage stage) const {
    return to_U16(_renderBinStack[to_base(stage)].size());
}

bool RenderBin::empty(RenderStage stage) const {
    return getBinSize(stage) == 0;
}

};
