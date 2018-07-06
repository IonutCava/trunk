#include "Headers/RenderBin.h"

#include "Core/Headers/Console.h"
#include "Utility/Headers/Localization.h"
#include "Graphs/Headers/SceneGraphNode.h"
#include "Managers/Headers/LightManager.h"
#include "Geometry/Shapes/Headers/Object3D.h"
#include "Geometry/Material/Headers/Material.h"
#include "Platform/Video/Headers/GFXDevice.h"
#include "Platform/Video/Headers/RenderStateBlock.h"

namespace Divide {

RenderBinItem::RenderBinItem(I32 sortKeyA,
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
    _stateHash = GFX_DEVICE.getStateBlockDescriptor(
                                nodeMaterial->getRenderStateBlock(
                                    GFX_DEVICE.getRenderStage())).getHash();
}

struct RenderQueueDistanceBacktoFront {
    bool operator()(const RenderBinItem& a, const RenderBinItem& b) const {
        return a._distanceToCameraSq < b._distanceToCameraSq;
    }
};

struct RenderQueueDistanceFrontToBack {
    bool operator()(const RenderBinItem& a, const RenderBinItem& b) const {
        return a._distanceToCameraSq > b._distanceToCameraSq;
    }
};

/// Sorting opaque items is a 3 step process:
/// 1: sort by shaders
/// 2: if the shader is identical, sort by state hash
/// 3: if shader is identical and state hash is identical, sort by albedo ID
struct RenderQueueKeyCompare {
    // Sort
    bool operator()(const RenderBinItem& a, const RenderBinItem& b) const {
        // Sort by shader in all states The sort key is the shader id (for now)
        if (a._sortKeyA < b._sortKeyA) {
            return true;
        }

        if (a._sortKeyA > b._sortKeyA) {
            return false;
        }

        // If the shader values are the same, we use the state hash for sorting
        // The _stateHash is a CRC value created based on the RenderState.
        if (a._stateHash < b._stateHash) {
            return true;
        }

        if (a._stateHash > b._stateHash) {
            return false;
        }

        // If both the shader are the same and the state hashes match,
        // we sort by the secondary key (usually the texture id)
        return (a._sortKeyB < b._sortKeyB);
    }
};

RenderBin::RenderBin(const RenderBinType& rbType,
                     const RenderingOrder::List& renderOrder, D32 drawKey)
    : _rbType(rbType), _renderOrder(renderOrder), _drawKey(drawKey) {
    _renderBinStack.reserve(125);
    renderBinTypeToNameMap[to_uint(RenderBinType::COUNT)] =
        "Invalid Bin";
    renderBinTypeToNameMap[to_uint(RenderBinType::RBT_MESH)] = "Mesh Bin";
    renderBinTypeToNameMap[to_uint(RenderBinType::RBT_IMPOSTOR)] =
        "Impostor Bin";
    renderBinTypeToNameMap[to_uint(RenderBinType::RBT_DELEGATE)] =
        "Delegate Bin";
    renderBinTypeToNameMap[to_uint(RenderBinType::RBT_TRANSLUCENT)] =
        "Translucent Bin";
    renderBinTypeToNameMap[to_uint(RenderBinType::RBT_SKY)] = "Sky Bin";
    renderBinTypeToNameMap[to_uint(RenderBinType::RBT_WATER)] =
        "Water Bin";
    renderBinTypeToNameMap[to_uint(RenderBinType::RBT_TERRAIN)] =
        "Terrain Bin";
    renderBinTypeToNameMap[to_uint(RenderBinType::RBT_PARTICLES)] =
        "Particle Bin";
    renderBinTypeToNameMap[to_uint(RenderBinType::RBT_VEGETATION_GRASS)] =
        "Grass Bin";
    renderBinTypeToNameMap[to_uint(RenderBinType::RBT_VEGETATION_TREES)] =
        "Trees Bin";
    renderBinTypeToNameMap[to_uint(RenderBinType::RBT_DECALS)] =
        "Decals Bin";
    renderBinTypeToNameMap[to_uint(RenderBinType::RBT_SHADOWS)] =
        "Shadow Bin";
}

void RenderBin::sort(U32 binIndex, RenderStage renderStage) {
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
                          std::end(_renderBinStack), RenderQueueKeyCompare());
            }
        } break;
        case RenderingOrder::List::BACK_TO_FRONT: {
            std::sort(std::begin(_renderBinStack), std::end(_renderBinStack),
                      RenderQueueDistanceBacktoFront());
        } break;
        case RenderingOrder::List::FRONT_TO_BACK: {
            std::sort(std::begin(_renderBinStack), std::end(_renderBinStack),
                      RenderQueueDistanceFrontToBack());
        } break;
        case RenderingOrder::List::NONE: {
            // no need to sort
        } break;
        case RenderingOrder::List::COUNT: {
            Console::errorfn(Locale::get("ERROR_INVALID_RENDER_BIN_SORT_ORDER"),
                             renderBinTypeToNameMap[to_uint(_rbType)]);
        } break;
    };

    U32 index = 0;
    for (RenderBinItem& item : _renderBinStack) {
        RenderingCompRenderBinAttorney::drawOrder(*item._renderable,
                                                  binIndex + index++);
    }
}

void RenderBin::refresh() {
    // WriteLock w_lock(_renderBinGetMutex);
    _renderBinStack.resize(0);
    _renderBinStack.reserve(128);
}

void RenderBin::addNodeToBin(SceneGraphNode& sgn, const vec3<F32>& eyePos) {
    I32 keyA = (U32)_renderBinStack.size() + 1;
    I32 keyB = keyA;
    RenderingComponent* const renderable =
        sgn.getComponent<RenderingComponent>();

    Material* nodeMaterial = renderable->getMaterialInstance();
    if (nodeMaterial) {
        nodeMaterial->getSortKeys(keyA, keyB);
    }

    _renderBinStack.push_back(RenderBinItem(
        keyA, keyB,
        sgn.getBoundingBoxConst().nearestDistanceFromPointSquared(eyePos),
        *renderable));
}

void RenderBin::preRender(RenderStage renderStage) {}

void RenderBin::render(const SceneRenderState& renderState,
                       RenderStage renderStage) {
    // We need to apply different materials for each stage. As nodes are sorted,
    // this should be very fast
    for (const RenderBinItem& item : _renderBinStack) {
        GFX_DEVICE.addToRenderQueue(
            RenderingCompRenderBinAttorney::getRenderData(
                *item._renderable, renderState, renderStage, true));
    }

    GFX_DEVICE.flushRenderQueue();
}

void RenderBin::postRender(const SceneRenderState& renderState,
                           RenderStage renderStage) {
    for (const RenderBinItem& item : _renderBinStack) {
        RenderingCompRenderBinAttorney::postDraw(*item._renderable, renderState,
                                                 renderStage);
    }
}
};