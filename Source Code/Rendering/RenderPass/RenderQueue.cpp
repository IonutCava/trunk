#include "Headers/RenderQueue.h"

#include "Headers/RenderBinMesh.h"
#include "Headers/RenderBinDelegate.h"
#include "Headers/RenderBinTranslucent.h"
#include "Headers/RenderBinParticles.h"

#include "Core/Headers/Console.h"
#include "Utility/Headers/Localization.h"
#include "Graphs/Headers/SceneGraphNode.h"
#include "Platform/Video/Headers/RenderStateBlock.h"
#include "Geometry/Shapes/Headers/Object3D.h"
#include "Geometry/Material/Headers/Material.h"

namespace Divide {

RenderQueue::RenderQueue() : _renderQueueLocked(false), _isSorted(false) {
    //_renderBins.reserve(RenderBin::COUNT);
}

RenderQueue::~RenderQueue() { MemoryManager::DELETE_HASHMAP(_renderBins); }

U16 RenderQueue::getRenderQueueStackSize() const {
    U16 temp = 0;
    for (const RenderBinMap::value_type& renderBins : _renderBins) {
        temp += (renderBins.second)->getBinSize();
    }
    return temp;
}

RenderBin* RenderQueue::getBin(RenderBin::RenderBinType rbType) {
    RenderBin* temp = nullptr;
    RenderBinMap::const_iterator binMapiter = _renderBins.find(rbType);
    if (binMapiter != std::end(_renderBins)) {
        temp = binMapiter->second;
    }
    return temp;
}

SceneGraphNode* RenderQueue::getItem(U16 renderBin, U16 index) {
    SceneGraphNode* temp = nullptr;
    if (renderBin < _renderBins.size()) {
        RenderBin* tempBin = _renderBins[_renderBinID[renderBin]];
        if (index < tempBin->getBinSize()) {
            temp = tempBin->getItem(index)._node;
        }
    }
    return temp;
}

RenderBin* RenderQueue::getOrCreateBin(const RenderBin::RenderBinType& rbType) {
    RenderBinMap::const_iterator binMapiter = _renderBins.find(rbType);
    if (binMapiter != std::end(_renderBins)) {
        return binMapiter->second;
    }

    RenderBin* temp = nullptr;

    switch (rbType) {
        case RenderBin::RenderBinType::RBT_MESH: {
            // By state varies based on the current rendering stage
            temp = MemoryManager_NEW RenderBinMesh(
                RenderBin::RenderBinType::RBT_MESH,
                RenderingOrder::List::BY_STATE, 0.0f);
        } break;
        case RenderBin::RenderBinType::RBT_TERRAIN: {
            temp = MemoryManager_NEW RenderBinDelegate(
                RenderBin::RenderBinType::RBT_TERRAIN,
                RenderingOrder::List::FRONT_TO_BACK, 1.0f);
        } break;
        case RenderBin::RenderBinType::RBT_DELEGATE: {
            temp = MemoryManager_NEW RenderBinDelegate(
                RenderBin::RenderBinType::RBT_DELEGATE,
                RenderingOrder::List::FRONT_TO_BACK, 2.0f);
        } break;
        case RenderBin::RenderBinType::RBT_SHADOWS: {
            temp = MemoryManager_NEW RenderBinDelegate(
                RenderBin::RenderBinType::RBT_SHADOWS,
                RenderingOrder::List::NONE, 3.0f);
        } break;
        case RenderBin::RenderBinType::RBT_DECALS: {
            temp = MemoryManager_NEW RenderBinMesh(
                RenderBin::RenderBinType::RBT_DECALS,
                RenderingOrder::List::FRONT_TO_BACK, 4.0f);
        } break;
        case RenderBin::RenderBinType::RBT_SKY: {
            // Draw sky after opaque but before translucent to prevent overdraw
            temp = MemoryManager_NEW RenderBinDelegate(
                RenderBin::RenderBinType::RBT_SKY, RenderingOrder::List::NONE,
                5.0f);
        } break;
        case RenderBin::RenderBinType::RBT_WATER: {
            // Water does not count as translucency, because rendering is very
            // specific
            temp = MemoryManager_NEW RenderBinDelegate(
                RenderBin::RenderBinType::RBT_WATER,
                RenderingOrder::List::BACK_TO_FRONT, 6.0f);
        } break;
        case RenderBin::RenderBinType::RBT_VEGETATION_GRASS: {
            temp = MemoryManager_NEW RenderBinDelegate(
                RenderBin::RenderBinType::RBT_VEGETATION_GRASS,
                RenderingOrder::List::BACK_TO_FRONT, 7.0f);
        } break;
        case RenderBin::RenderBinType::RBT_VEGETATION_TREES: {
            temp = MemoryManager_NEW RenderBinDelegate(
                RenderBin::RenderBinType::RBT_VEGETATION_TREES,
                RenderingOrder::List::BACK_TO_FRONT, 7.5f);
        } break;
        case RenderBin::RenderBinType::RBT_PARTICLES: {
            temp = MemoryManager_NEW RenderBinParticles(
                RenderBin::RenderBinType::RBT_PARTICLES,
                RenderingOrder::List::BACK_TO_FRONT, 8.0f);
        } break;
        case RenderBin::RenderBinType::RBT_TRANSLUCENT: {
            // When rendering translucent objects we should sort each object's
            // polygons depending on it's distance to the camera
            temp = MemoryManager_NEW RenderBinTranslucent(
                RenderBin::RenderBinType::RBT_TRANSLUCENT,
                RenderingOrder::List::BACK_TO_FRONT, 9.0f);
        } break;
        case RenderBin::RenderBinType::RBT_IMPOSTOR: {
            temp = MemoryManager_NEW RenderBinDelegate(
                RenderBin::RenderBinType::RBT_IMPOSTOR,
                RenderingOrder::List::FRONT_TO_BACK, 9.9f);
        } break;
        default:
        case RenderBin::RenderBinType::COUNT: {
            Console::errorfn(Locale::get("ERROR_INVALID_RENDER_BIN_CREATION"));
        } break;
    };

    hashAlg::emplace(_renderBins, rbType, temp);
    hashAlg::emplace(_renderBinID, static_cast<U16>(_renderBins.size() - 1),
                     rbType);

    _sortedRenderBins.insert(
        std::lower_bound(
            std::begin(_sortedRenderBins), std::end(_sortedRenderBins), temp,
            [](RenderBin* const a, const RenderBin* const b)
                -> bool { return a->getSortOrder() < b->getSortOrder(); }),
        temp);
    _isSorted = false;

    return temp;
}

RenderBin* RenderQueue::getBinForNode(SceneNode* const node,
                                      Material* const matInstance) {
    assert(node != nullptr);
    switch (node->getType()) {
        case SceneNodeType::TYPE_LIGHT: {
            return getOrCreateBin(RenderBin::RenderBinType::RBT_IMPOSTOR);
        }
        case SceneNodeType::TYPE_WATER: {
            return getOrCreateBin(RenderBin::RenderBinType::RBT_WATER);
        }
        case SceneNodeType::TYPE_PARTICLE_EMITTER: {
            return getOrCreateBin(RenderBin::RenderBinType::RBT_PARTICLES);
        }
        case SceneNodeType::TYPE_VEGETATION_GRASS: {
            return getOrCreateBin(
                RenderBin::RenderBinType::RBT_VEGETATION_GRASS);
        }
        case SceneNodeType::TYPE_VEGETATION_TREES: {
            return getOrCreateBin(
                RenderBin::RenderBinType::RBT_VEGETATION_TREES);
        }
        case SceneNodeType::TYPE_SKY: {
            return getOrCreateBin(RenderBin::RenderBinType::RBT_SKY);
        }
        case SceneNodeType::TYPE_OBJECT3D: {
            if (static_cast<Object3D*>(node)->getObjectType() ==
                Object3D::ObjectType::TERRAIN) {
                return getOrCreateBin(RenderBin::RenderBinType::RBT_TERRAIN);
            }
            // Check if the object has a material with translucency
            if (matInstance && matInstance->isTranslucent()) {
                // Add it to the appropriate bin if so ...
                return getOrCreateBin(
                    RenderBin::RenderBinType::RBT_TRANSLUCENT);
            }
            //... else add it to the general geometry bin
            return getOrCreateBin(RenderBin::RenderBinType::RBT_MESH);
        }
    }
    return nullptr;
}

void RenderQueue::addNodeToQueue(SceneGraphNode& sgn, const vec3<F32>& eyePos) {
    RenderingComponent* renderingCmp = sgn.getComponent<RenderingComponent>();
    RenderBin* rb = getBinForNode(
        sgn.getNode(),
        renderingCmp ? renderingCmp->getMaterialInstance() : nullptr);
    if (rb) {
        rb->addNodeToBin(sgn, eyePos);
    }
    _isSorted = false;
}

void RenderQueue::sort(RenderStage currentRenderStage) {
    /*if(_renderQueueLocked && _isSorted) {
        return;
    }*/

    U32 index = 0;
    for (RenderBinMap::value_type& renderBin : _renderBins) {
        renderBin.second->sort(index, currentRenderStage);
        index += renderBin.second->getBinSize();
    }

    _isSorted = true;
}

void RenderQueue::refresh(bool force) {
    if (_renderQueueLocked && !force) {
        return;
    }
    for (RenderBinMap::value_type& renderBin : _renderBins) {
        renderBin.second->refresh();
    }
    _isSorted = false;
}

void RenderQueue::lock() { _renderQueueLocked = true; }

void RenderQueue::unlock(bool resetNodes) {
    _renderQueueLocked = false;

    if (resetNodes) {
        refresh();
    }
}
};