#include "stdafx.h"

#include "Headers/RenderBin.h"

#include "Core/Headers/Console.h"
#include "Utility/Headers/Localization.h"
#include "Graphs/Headers/SceneGraphNode.h"
#include "Geometry/Shapes/Headers/Object3D.h"
#include "Geometry/Material/Headers/Material.h"
#include "Platform/Video/Headers/GFXDevice.h"
#include "Managers/Headers/RenderPassManager.h"
#include "ECS/Components/Headers/BoundsComponent.h"
#include "ECS/Components/Headers/RenderingComponent.h"

namespace Divide {

namespace {
    constexpr U32 AVERAGE_BIN_SIZE = 127;

    auto RenderQueueDistanceBackToFront = [](const RenderBinItem& a, const RenderBinItem& b) -> bool {
        return a._distanceToCameraSq > b._distanceToCameraSq;
    };

    auto RenderQueueDistanceFrontToBack = [](const RenderBinItem& a, const RenderBinItem& b) -> bool {
        return a._distanceToCameraSq < b._distanceToCameraSq;
    };

    auto RenderQueueWaterFirst = [](const RenderBinItem& a, const RenderBinItem& b) -> bool {
        return a._renderable->getSGN().getNode().type() == SceneNodeType::TYPE_WATER;
    };

    /// Sorting opaque items is a 3 step process:
    /// 1: sort by shaders
    /// 2: if the shader is identical, sort by state hash
    /// 3: if shader is identical and state hash is identical, sort by albedo ID
    auto RenderQueueKeyCompare = [](const RenderBinItem& a, const RenderBinItem& b) -> bool {
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
        // Final fallback is front to back
        return RenderQueueDistanceFrontToBack(a, b);
    };
};

RenderBin::RenderBin(RenderBinType rbType) : _rbType(rbType)
{
    for (RenderBinStack& stack : _renderBinStack) {
        stack.reserve(AVERAGE_BIN_SIZE);
    }
}

RenderBin::~RenderBin()
{
}

const RenderBinItem& RenderBin::getItem(RenderStage stage, U16 index) const {
    assert(index < _renderBinStack[to_base(stage)].size());
    return _renderBinStack[to_base(stage)][index];
}

void RenderBin::sort(RenderStage stage, RenderingOrder renderOrder) {
    // Lock w_lock(_renderBinGetMutex);
    RenderBinStack& stack = _renderBinStack[to_U8(stage)];

    switch (renderOrder) {
        case RenderingOrder::BY_STATE: {
            eastl::sort(eastl::begin(stack), eastl::end(stack), RenderQueueKeyCompare);
        } break;
        case RenderingOrder::BACK_TO_FRONT: {
            eastl::sort(eastl::begin(stack), eastl::end(stack), RenderQueueDistanceBackToFront);
        } break;
        case RenderingOrder::FRONT_TO_BACK: {
            eastl::sort(eastl::begin(stack), eastl::end(stack), RenderQueueDistanceFrontToBack);
        } break;
        case RenderingOrder::WATER_FIRST: {
            eastl::sort(eastl::begin(stack), eastl::end(stack), RenderQueueWaterFirst);
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

void RenderBin::sort(RenderStage stage, RenderingOrder renderOrder, const Task& parentTask) {
    OPTICK_EVENT();

    ACKNOWLEDGE_UNUSED(parentTask);
    sort(stage, renderOrder);
}

void RenderBin::getSortedNodes(RenderStage stage, SortedQueue& nodes, U16& countOut) const {
    OPTICK_EVENT();

    nodes.resize(0);
    nodes.reserve(getBinSize(stage));

    const RenderBinStack& stack = _renderBinStack[to_base(stage)];
    for (const RenderBinItem& item : stack) {
        nodes.emplace_back(item._renderable);
    }

    countOut += to_U16(stack.size());
}

void RenderBin::refresh(RenderStage stage) {
    // Lock w_lock(_renderBinGetMutex);
    _renderBinStack[to_base(stage)].resize(0);
    _renderBinStack[to_base(stage)].reserve(AVERAGE_BIN_SIZE);
}

void RenderBin::addNodeToBin(const SceneGraphNode& sgn, RenderStagePass stagePass, F32 minDistToCameraSq) {
    RenderingComponent* const rComp = sgn.get<RenderingComponent>();
    const U8 stageIndex = to_U8(stagePass._stage);
    // Sort by state hash depending on the current rendering stage
    // Save the render state hash value for sorting
    const size_t sortHash = rComp->getSortKeyHash(stagePass);
    RenderBinItem& item = _renderBinStack[stageIndex].emplace_back(rComp, 0, 0, sortHash, minDistToCameraSq);

    const Material_ptr& nodeMaterial = rComp->getMaterialInstance();
    if (nodeMaterial) {
        nodeMaterial->getSortKeys(stagePass, item._sortKeyA, item._sortKeyB);
    } else {
        item._sortKeyA = to_I64(_renderBinStack[stageIndex].size());
        item._sortKeyB = to_I32(item._sortKeyA);
    }
}

void RenderBin::populateRenderQueue(RenderStagePass stagePass, vectorEASTLFast<RenderPackage*>& queueInOut) const {
    OPTICK_EVENT();

    for (const RenderBinItem& item : _renderBinStack[to_base(stagePass._stage)]) {
        queueInOut.push_back(&item._renderable->getDrawPackage(stagePass));
    }
}

void RenderBin::postRender(const SceneRenderState& renderState, RenderStagePass stagePass, GFX::CommandBuffer& bufferInOut) {
    for (const RenderBinItem& item : _renderBinStack[to_base(stagePass._stage)]) {
        Attorney::RenderingCompRenderBin::postRender(*item._renderable, renderState, stagePass, bufferInOut);
    }
}

U16 RenderBin::getBinSize(RenderStage stage) const {
    return to_U16(_renderBinStack[to_base(stage)].size());
}

bool RenderBin::empty(RenderStage stage) const {
    return getBinSize(stage) == 0;
}

};
