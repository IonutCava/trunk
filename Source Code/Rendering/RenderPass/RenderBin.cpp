#include "Headers/RenderBin.h"

#include "Core/Headers/Console.h"
#include "Utility/Headers/Localization.h"
#include "Graphs/Headers/SceneGraphNode.h"
#include "Managers/Headers/LightManager.h"
#include "Geometry/Shapes/Headers/Object3D.h"
#include "Geometry/Material/Headers/Material.h"
#include "Platform/Video/Headers/GFXDevice.h"

namespace Divide {

RenderBinItem::RenderBinItem(RenderStage currentStage, 
                             I32 sortKeyA,
                             I32 sortKeyB,
                             F32 distToCamSq,
                             RenderingComponent& renderable)
    : _renderable(&renderable),
      _sortKeyA(sortKeyA),
      _sortKeyB(sortKeyB),
      _distanceToCameraSq(distToCamSq)
{
    _stateHash = 0;
    Material* nodeMaterial = _renderable->getMaterialInstance();
    // If we do not have a material, no need to continue
    if (!nodeMaterial) {
        return;
    }
    // Sort by state hash depending on the current rendering stage
    // Save the render state hash value for sorting
    _stateHash = renderable.getDrawStateHash(currentStage);
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

RenderBin::RenderBin(RenderBinType rbType,
                     RenderingOrder::List renderOrder)
    : _binIndex(0),
      _rbType(rbType),
      _binPropertyMask(0),
      _renderQueueIndex(-1),
      _renderOrder(renderOrder)
{
    _renderBinStack.reserve(125);

    if(_rbType != +RenderBinType::RBT_OPAQUE &&  _rbType != +RenderBinType::RBT_TERRAIN)  {
        SetBit(_binPropertyMask, to_const_uint(RenderBitProperty::TRANSLUCENT));
    } else {
        ClearBit(_binPropertyMask, to_const_uint(RenderBitProperty::TRANSLUCENT));
    }
}

void RenderBin::sort(bool stopRequested, RenderStage renderStage) {
    // WriteLock w_lock(_renderBinGetMutex);
    switch (_renderOrder) {
        default:
        case RenderingOrder::List::BY_STATE: {
            if (GFX_DEVICE.isDepthStage()) {
                std::sort(std::begin(_renderBinStack),
                          std::end(_renderBinStack),
                          RenderQueueDistanceFrontToBack());
            } else {
                std::sort(std::begin(_renderBinStack),
                          std::end(_renderBinStack),
                          RenderQueueKeyCompare());
            }
        } break;
        case RenderingOrder::List::BACK_TO_FRONT: {
            std::sort(std::begin(_renderBinStack),
                      std::end(_renderBinStack),
                      RenderQueueDistanceBackToFront());
        } break;
        case RenderingOrder::List::FRONT_TO_BACK: {
            std::sort(std::begin(_renderBinStack),
                      std::end(_renderBinStack),
                      RenderQueueDistanceFrontToBack());
        } break;
        case RenderingOrder::List::NONE: {
            // no need to sort
        } break;
        case RenderingOrder::List::COUNT: {
            Console::errorfn(Locale::get(_ID("ERROR_INVALID_RENDER_BIN_SORT_ORDER")),
                             _rbType._to_string());
        } break;
    };

    U32 index = _binIndex;
    for (RenderBinItem& item : _renderBinStack) {
        Attorney::RenderingCompRenderBin::drawOrder(*item._renderable, index++);
    }
}

void RenderBin::refresh() {
    // WriteLock w_lock(_renderBinGetMutex);
    _renderBinStack.resize(0);
    _renderBinStack.reserve(128);
}

void RenderBin::addNodeToBin(const SceneGraphNode& sgn, RenderStage stage, const vec3<F32>& eyePos) {
    I32 keyA = to_uint(_renderBinStack.size() + 1);
    I32 keyB = keyA;

    RenderingComponent* const renderable = sgn.get<RenderingComponent>();

    Material* nodeMaterial = renderable->getMaterialInstance();
    if (nodeMaterial) {
        nodeMaterial->getSortKeys(keyA, keyB);
    }

    vectorAlg::emplace_back(_renderBinStack, 
                            stage,
                            keyA,
                            keyB,
                            sgn.get<BoundsComponent>()->getBoundingBox().nearestDistanceFromPointSquared(eyePos),
                            *renderable);
}

void RenderBin::populateRenderQueue(RenderStage renderStage) {
    GFXDevice& gfx = GFX_DEVICE;

    _renderQueueIndex = gfx.reserveRenderQueue();

    if (_renderQueueIndex == -1) {
        return;
    }

    // We need to apply different materials for each stage. As nodes are sorted, this should be very fast
    for (const RenderBinItem& item : _renderBinStack) {
        gfx.addToRenderQueue(_renderQueueIndex,
                             Attorney::RenderingCompRenderBin::getRenderData(*item._renderable, renderStage));
    }
}

void RenderBin::postRender(const SceneRenderState& renderState, RenderStage renderStage) {
    if (_renderQueueIndex == -1) {
        return;
    }

    for (const RenderBinItem& item : _renderBinStack) {
        Attorney::RenderingCompRenderBin::postRender(*item._renderable, renderState, renderStage);
    }
}
};
