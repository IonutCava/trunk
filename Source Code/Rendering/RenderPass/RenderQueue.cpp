#include "Headers/RenderQueue.h"

#include "Core/Headers/Console.h"
#include "Core/Headers/TaskPool.h"
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
    _activeBins.clear();
}

U16 RenderQueue::getRenderQueueStackSize() const {
    U16 temp = 0;
    for (RenderBin* bin : _activeBins) {
        temp += bin->getBinSize();
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
    // Bins are sorted by their type
    _renderBins[rbType._to_integral()] = temp;
    
    _activeBins.resize(0);
    for (RenderBin* bin : _renderBins) {
        if (bin != nullptr) {
            _activeBins.push_back(bin);
        }
    }

    return temp;
}

RenderBin* RenderQueue::getBinForNode(const std::shared_ptr<SceneNode>& node,
                                      const Material_ptr& matInstance) {
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
            if (static_cast<Object3D*>(node.get())->getObjectType() ==
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

void RenderQueue::addNodeToQueue(const SceneGraphNode& sgn, RenderStage stage, const vec3<F32>& eyePos) {
    static Material_ptr defaultMat;

    RenderingComponent* const renderingCmp = sgn.get<RenderingComponent>();
    RenderBin* rb = getBinForNode(sgn.getNode(),
                                  renderingCmp
                                    ? renderingCmp->getMaterialInstance()
                                    : defaultMat);
    if (rb) {
        rb->addNodeToBin(sgn, stage, eyePos);
    }
}

void RenderQueue::populateRenderQueues(RenderStage renderStage) {
    TaskHandle populateTask = CreateTask(DELEGATE_CBK_PARAM<bool>());
    for (RenderBin* renderBin : _activeBins) {
        if (!renderBin->empty()) {
            populateTask.addChildTask(CreateTask(DELEGATE_BIND(&RenderBin::populateRenderQueue,
                                                               renderBin,
                                                               std::placeholders::_1,
                                                               renderStage))._task)->startTask(Task::TaskPriority::HIGH);      
        }
    }
    populateTask.startTask(Task::TaskPriority::MAX);
    populateTask.wait();
}

void RenderQueue::postRender(const SceneRenderState& renderState, RenderStage renderStage, RenderSubPassCmds& subPassesInOut) {
    for (RenderBin* renderBin : _activeBins) {
        renderBin->postRender(renderState, renderStage, subPassesInOut);
    }
}

void RenderQueue::sort(RenderStage renderStage) {
    U32 index = 0;
    for (RenderBin* renderBin : _activeBins) {
        renderBin->binIndex(index);
        index += renderBin->getBinSize();
    }

    TaskHandle sortTask = CreateTask(DELEGATE_CBK_PARAM<bool>());
    for (RenderBin* renderBin : _activeBins) {
        if (!renderBin->empty()) {
            sortTask.addChildTask(CreateTask(DELEGATE_BIND(&RenderBin::sort,
                                                        renderBin,
                                                        std::placeholders::_1,
                                                        renderStage))._task)->startTask(Task::TaskPriority::HIGH);
        }
    }
    sortTask.startTask(Task::TaskPriority::MAX);
    sortTask.wait();
}

void RenderQueue::refresh() {
    for (RenderBin* renderBin : _activeBins) {
        renderBin->refresh();
    }
}

};