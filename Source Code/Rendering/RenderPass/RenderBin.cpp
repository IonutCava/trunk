#include "stdafx.h"

#include "Headers/RenderBin.h"

#include "Core/Headers/Console.h"
#include "Utility/Headers/Localization.h"
#include "Graphs/Headers/SceneGraphNode.h"
#include "Geometry/Shapes/Headers/Object3D.h"
#include "Geometry/Material/Headers/Material.h"
#include "Platform/Video/Headers/GFXDevice.h"

namespace Divide {

namespace {
    constexpr U32 AVERAGE_BIN_SIZE = 127;
};

RenderBinItem::RenderBinItem(RenderStagePass currentStage,
                             I32 sortKeyA,
                             I32 sortKeyB,
                             F32 distToCamSq,
                             RenderingComponent& renderable)
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

RenderBin::RenderBin(GFXDevice& context, RenderBinType rbType)
    : _context(context),
      _rbType(rbType)
{
    for (RenderBinStack& stack : _renderBinStack) {
        stack.reserve(AVERAGE_BIN_SIZE);
    }
}

RenderBin::~RenderBin()
{
}

const RenderBinItem& RenderBin::getItem(RenderStagePass stagePass, U16 index) const {
    assert(index < _renderBinStack[to_base(stagePass._stage)].size());
    return _renderBinStack[to_base(stagePass._stage)][index];
}

void RenderBin::sort(RenderStagePass stagePass, RenderingOrder::List renderOrder) {
    // WriteLock w_lock(_renderBinGetMutex);
    U8 stageIndex = to_U8(stagePass._stage);

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

void RenderBin::sort(RenderStagePass stagePass, RenderingOrder::List renderOrder, const Task& parentTask) {
    ACKNOWLEDGE_UNUSED(parentTask);
    sort(stagePass, renderOrder);
}

void RenderBin::getSortedNodes(RenderStagePass stagePass, vectorEASTL<SceneGraphNode*>& nodes) const {
    nodes.resize(0);
    for (const RenderBinItem& item : _renderBinStack[to_base(stagePass._stage)]) {
        nodes.push_back(&(item._renderable->getSGN()));
    }
}

void RenderBin::refresh(RenderStagePass stagePass) {
    // WriteLock w_lock(_renderBinGetMutex);
    _renderBinStack[to_base(stagePass._stage)].resize(0);
    _renderBinStack[to_base(stagePass._stage)].reserve(AVERAGE_BIN_SIZE);
}

void RenderBin::addNodeToBin(const SceneGraphNode& sgn, RenderStagePass stagePass, const vec3<F32>& eyePos) {
    U8 stageIndex = to_U8(stagePass._stage);

    I32 keyA = to_U32(_renderBinStack[stageIndex].size() + 1);
    I32 keyB = keyA;

    RenderingComponent* const renderable = sgn.get<RenderingComponent>();

    const Material_ptr& nodeMaterial = renderable->getMaterialInstance();
    if (nodeMaterial) {
        nodeMaterial->getSortKeys(stagePass, keyA, keyB);
    }

    _renderBinStack[stageIndex].emplace_back(stagePass,
                                             keyA,
                                             keyB,
                                             sgn.get<BoundsComponent>()->getBoundingBox().nearestDistanceFromPointSquared(eyePos),
                                             *renderable);
}

void RenderBin::populateRenderQueue(RenderStagePass stagePass) {
    // We need to apply different materials for each stage. As nodes are sorted, this should be very fast
    RenderBinType type = getType();
    _context.lockQueue(type);
    _context.clearRenderQueue(type);
    for (const RenderBinItem& item : _renderBinStack[to_base(stagePass._stage)]) {
        _context.addToRenderQueue(type, Attorney::RenderingCompRenderBin::getRenderData(*item._renderable, stagePass));
    }
    _context.unlockQueue(type);
}

void RenderBin::postRender(const SceneRenderState& renderState, RenderStagePass stagePass, GFX::CommandBuffer& bufferInOut) {
    for (const RenderBinItem& item : _renderBinStack[to_base(stagePass._stage)]) {
        Attorney::RenderingCompRenderBin::postRender(*item._renderable, renderState, stagePass, bufferInOut);
    }
}

U16 RenderBin::getBinSize(RenderStagePass stagePass) const {
    return (U16)_renderBinStack[to_base(stagePass._stage)].size();
}

bool RenderBin::empty(RenderStagePass stagePass) const {
    return getBinSize(stagePass) == 0;
}

};
