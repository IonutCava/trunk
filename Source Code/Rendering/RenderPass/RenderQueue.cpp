#include "Headers/RenderQueue.h"

#include "Core/Headers/Console.h"
#include "Utility/Headers/Localization.h"
#include "Graphs/Headers/SceneGraphNode.h"
#include "Geometry/Shapes/Headers/Object3D.h"
#include "Geometry/Material/Headers/Material.h"

namespace Divide {

RenderQueue::RenderQueue()
{
    _renderBins.fill(nullptr);
}

RenderQueue::~RenderQueue()
{
    for (RenderBin* bin : _renderBins) {
        MemoryManager::DELETE(bin);
    }
    _renderBins.fill(nullptr);
}

U16 RenderQueue::getRenderQueueStackSize() const {
    U16 temp = 0;
    for (RenderBin* bin : _renderBins) {
        if (bin != nullptr) {
            temp += bin->getBinSize();
        }
    }
    return temp;
}

RenderBin* RenderQueue::getOrCreateBin(RenderBinType rbType) {
    RenderBin* temp = getBin(rbType);
    if (temp != nullptr) {
        return temp;
    }

    RenderingOrder::List sortOrder = RenderingOrder::List::COUNT;
    switch (rbType) {
        case RenderBinType::RBT_OPAQUE: {
            // By state varies based on the current rendering stage
            sortOrder = RenderingOrder::List::BY_STATE;
        } break;
        case RenderBinType::RBT_SKY: {
            sortOrder = RenderingOrder::List::NONE;
        } break;
        case RenderBinType::RBT_IMPOSTOR:
        case RenderBinType::RBT_TERRAIN:
        case RenderBinType::RBT_DECALS: {
            sortOrder = RenderingOrder::List::FRONT_TO_BACK;
        } break;
        case RenderBinType::RBT_WATER:
        case RenderBinType::RBT_VEGETATION_GRASS:
        case RenderBinType::RBT_PARTICLES:
        case RenderBinType::RBT_TRANSLUCENT: {
            sortOrder = RenderingOrder::List::BACK_TO_FRONT;
        } break;

        default: {
            Console::errorfn(Locale::get(_ID("ERROR_INVALID_RENDER_BIN_CREATION")));
            return nullptr;
        } break;
    };

    temp = MemoryManager_NEW RenderBin(rbType, sortOrder);
    _renderBins[rbType._to_integral()] = temp;
    
    return temp;
}

RenderBin* RenderQueue::getBinForNode(SceneNode* const node,
                                      Material* const matInstance) {
    assert(node != nullptr);
    switch (node->getType()) {
        case SceneNodeType::TYPE_LIGHT: {
            return getOrCreateBin(RenderBinType::RBT_IMPOSTOR);
        }
        case SceneNodeType::TYPE_WATER: {
            return getOrCreateBin(RenderBinType::RBT_WATER);
        }
        case SceneNodeType::TYPE_PARTICLE_EMITTER: {
            return getOrCreateBin(RenderBinType::RBT_PARTICLES);
        }
        case SceneNodeType::TYPE_VEGETATION_GRASS: {
            return getOrCreateBin(RenderBinType::RBT_VEGETATION_GRASS);
        }
        case SceneNodeType::TYPE_SKY: {
            return getOrCreateBin(RenderBinType::RBT_SKY);
        }
        case SceneNodeType::TYPE_VEGETATION_TREES:
        case SceneNodeType::TYPE_OBJECT3D: {
            if (static_cast<Object3D*>(node)->getObjectType() ==
                Object3D::ObjectType::TERRAIN) {
                return getOrCreateBin(RenderBinType::RBT_TERRAIN);
            }
            // Check if the object has a material with translucency
            if (matInstance && matInstance->isTranslucent()) {
                // Add it to the appropriate bin if so ...
                return getOrCreateBin(RenderBinType::RBT_TRANSLUCENT);
            }
            //... else add it to the general geometry bin
            return getOrCreateBin(RenderBinType::RBT_OPAQUE);
        }
    }
    return nullptr;
}

void RenderQueue::addNodeToQueue(SceneGraphNode& sgn, const vec3<F32>& eyePos) {
    RenderingComponent* renderingCmp = sgn.getComponent<RenderingComponent>();
    RenderBin* rb = getBinForNode(sgn.getNode(),
                                  renderingCmp
                                    ? renderingCmp->getMaterialInstance()
                                    : nullptr);
    if (rb) {
        rb->addNodeToBin(sgn, eyePos);
    }
}

void RenderQueue::render(SceneRenderState& renderState,
                         RenderStage renderStage) {
    for (RenderBin* renderBin : _renderBins) {
        if (renderBin != nullptr) {
            renderBin->render(renderState, renderStage);
        }
    }
}

void RenderQueue::postRender(SceneRenderState& renderState,
                             RenderStage renderStage) {
    for (RenderBin* renderBin : _renderBins) {
        if (renderBin != nullptr) {
            renderBin->postRender(renderState, renderStage);
        }
    }
}

void RenderQueue::sort(RenderStage renderStage) {
    

    U32 index = 0;
    for (RenderBin* renderBin : _renderBins) {
        if (renderBin != nullptr) {
            renderBin->binIndex(index);
            index += renderBin->getBinSize();
        }
    }

    _sortingTasks.resize(0);
    for (RenderBin* renderBin : _renderBins) {
        if (renderBin != nullptr) {
            _sortingTasks.push_back(std::async(std::launch::async | std::launch::deferred,
                [renderBin, renderStage]() {
                    renderBin->sort(renderStage);
                }
            ));
        }
    }

    for (std::future<void>& task : _sortingTasks) {
        task.get();
    }
}

void RenderQueue::refresh() {
    for (RenderBin* renderBin : _renderBins) {
        if (renderBin != nullptr) {
            renderBin->refresh();
        }
    }
}

};