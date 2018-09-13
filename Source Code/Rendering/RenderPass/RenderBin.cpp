#include "stdafx.h"

#include "Headers/RenderBin.h"

#include "Core/Headers/Console.h"
#include "Utility/Headers/Localization.h"
#include "Graphs/Headers/SceneGraphNode.h"
#include "Geometry/Shapes/Headers/Object3D.h"
#include "Geometry/Material/Headers/Material.h"
#include "Platform/Video/Headers/GFXDevice.h"
#include "Managers/Headers/RenderPassManager.h"

namespace Divide {

namespace {
    constexpr U32 AVERAGE_BIN_SIZE = 127;
};

RenderBinItem::RenderBinItem(RenderStagePass currentStage,
                             I32 sortKeyA,
                             I32 sortKeyB,
                             F32 distToCamSq,
                             RenderingComponent& renderable) noexcept
    : _renderable(&renderable),
      _sortKeyA(sortKeyA),
      _sortKeyB(sortKeyB),
      _distanceToCameraSq(distToCamSq)
{
    // Sort by state hash depending on the current rendering stage
    // Save the render state hash value for sorting
    _stateHash = Attorney::RenderingCompRenderBin::getSortKeyHash(renderable, currentStage);
}

/// Sorting opaque items is a 3 step process:
/// 1: sort by shaders
/// 2: if the shader is identical, sort by state hash
/// 3: if shader is identical and state hash is identical, sort by albedo ID
struct RenderQueueKeyCompare {
    // Sort
    bool operator()(const RenderBinItem& a, const RenderBinItem& b) const {
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
        return a._sortKeyB < b._sortKeyB;
    }
};

struct RenderQueueDistanceBackToFront {
    bool operator()(const RenderBinItem& a, const RenderBinItem& b) const {
       return a._distanceToCameraSq > b._distanceToCameraSq;
    }
};

struct RenderQueueDistanceFrontToBack {
    bool operator()(const RenderBinItem& a, const RenderBinItem& b) const {
        return a._distanceToCameraSq < b._distanceToCameraSq;
    }
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

void RenderBin::sort(RenderStage stage, RenderingOrder::List renderOrder) {
    // Lock w_lock(_renderBinGetMutex);
    U8 stageIndex = to_U8(stage);

    switch (renderOrder) {
        case RenderingOrder::List::BY_STATE: {
            std::sort(std::begin(_renderBinStack[stageIndex]),
                      std::end(_renderBinStack[stageIndex]),
                      RenderQueueKeyCompare());
        } break;
        case RenderingOrder::List::BACK_TO_FRONT: {
            std::sort(std::begin(_renderBinStack[stageIndex]),
                      std::end(_renderBinStack[stageIndex]),
                      RenderQueueDistanceBackToFront());
        } break;
        case RenderingOrder::List::FRONT_TO_BACK: {
            std::sort(std::begin(_renderBinStack[stageIndex]),
                      std::end(_renderBinStack[stageIndex]),
                      RenderQueueDistanceFrontToBack());
        } break;
        case RenderingOrder::List::NONE: {
            // no need to sort
        } break;
        default:
        case RenderingOrder::List::COUNT: {
            Console::errorfn(Locale::get(_ID("ERROR_INVALID_RENDER_BIN_SORT_ORDER")), _rbType._to_string());
        } break;
    };
}

void RenderBin::sort(RenderStage stage, RenderingOrder::List renderOrder, const Task& parentTask) {
    ACKNOWLEDGE_UNUSED(parentTask);
    sort(stage, renderOrder);
}

void RenderBin::getSortedNodes(RenderStage stage, vectorEASTL<SceneGraphNode*>& nodes, U16& countOut) const {
    nodes.resize(0);
    nodes.reserve(getBinSize(stage));

    for (const RenderBinItem& item : _renderBinStack[to_base(stage)]) {
        nodes.push_back(&(item._renderable->getSGN()));
        ++countOut;
    }
}

void RenderBin::refresh(RenderStage stage) {
    // Lock w_lock(_renderBinGetMutex);
    _renderBinStack[to_base(stage)].resize(0);
    _renderBinStack[to_base(stage)].reserve(AVERAGE_BIN_SIZE);
}

void RenderBin::addNodeToBin(const SceneGraphNode& sgn, RenderStagePass stagePass, const vec3<F32>& eyePos) {
    U8 stageIndex = to_U8(stagePass._stage);

    I32 keyA = to_U32(_renderBinStack[stageIndex].size() + 1);
    I32 keyB = keyA;

    RenderingComponent* const rComp = sgn.get<RenderingComponent>();

    const Material_ptr& nodeMaterial = rComp->getMaterialInstance();
    if (nodeMaterial) {
        nodeMaterial->getSortKeys(stagePass, keyA, keyB);
    }

    _renderBinStack[stageIndex].emplace_back(stagePass,
                                             keyA,
                                             keyB,
                                             sgn.get<BoundsComponent>()->getBoundingBox().nearestDistanceFromPointSquared(eyePos),
                                             *rComp);
}

void RenderBin::populateRenderQueue(RenderStagePass stagePass, vectorEASTL<RenderPackage*>& queueInOut) const {
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
