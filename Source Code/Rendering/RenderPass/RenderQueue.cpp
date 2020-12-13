#include "stdafx.h"

#include "Headers/RenderQueue.h"

#include "Core/Headers/Kernel.h"
#include "Core/Headers/PlatformContext.h"
#include "ECS/Components/Headers/RenderingComponent.h"
#include "Geometry/Material/Headers/Material.h"
#include "Geometry/Shapes/Headers/Object3D.h"
#include "Graphs/Headers/SceneGraphNode.h"
#include "Managers/Headers/RenderPassManager.h"
#include "Utility/Headers/Localization.h"

namespace Divide {

RenderQueue::RenderQueue(Kernel& parent)
    : KernelComponent(parent),
      _renderBins{nullptr}
{
    for (const RenderBinType rbType : RenderBinType::_values()) {
        if (rbType._value == RenderBinType::RBT_COUNT) {
            continue;
        }

        _renderBins[rbType._value] = MemoryManager_NEW RenderBin(rbType);
    }
}

RenderQueue::~RenderQueue()
{
    for (RenderBin* bin : _renderBins) {
        MemoryManager::DELETE(bin);
    }
}

U16 RenderQueue::getRenderQueueStackSize(const RenderStage stage) const {
    U16 temp = 0;
    for (RenderBin* bin : _renderBins) {
        if (bin != nullptr) {
            temp += bin->getBinSize(stage);
        }
    }
    return temp;
}

RenderingOrder RenderQueue::getSortOrder(const RenderStagePass stagePass, const RenderBinType rbType) const {
    RenderingOrder sortOrder = RenderingOrder::COUNT;
    switch (rbType) {
        case RenderBinType::RBT_OPAQUE: {
            // Opaque items should be rendered front to back in depth passes for early-Z reasons
            sortOrder = stagePass.isDepthPass() ? RenderingOrder::FRONT_TO_BACK
                                                : RenderingOrder::BY_STATE;
        } break;
        case RenderBinType::RBT_SKY: {
            sortOrder = RenderingOrder::NONE;
        } break;
        case RenderBinType::RBT_IMPOSTOR:
        case RenderBinType::RBT_TERRAIN: {
            sortOrder = RenderingOrder::FRONT_TO_BACK;
        } break;
        case RenderBinType::RBT_TERRAIN_AUX: {
            // Water first, everything else after
            sortOrder = RenderingOrder::WATER_FIRST;
        } break;
        case RenderBinType::RBT_TRANSLUCENT: {
            // We are using weighted blended OIT. State is fine (and faster)
            // Use an override one level up from this if we need a regular forward-style pass
            sortOrder = RenderingOrder::BY_STATE;
        } break;
        default:
        case RenderBinType::RBT_COUNT: {
            Console::errorfn(Locale::get(_ID("ERROR_INVALID_RENDER_BIN_CREATION")));
        } break;
    };
    
    return sortOrder;
}

RenderBin* RenderQueue::getBinForNode(const SceneGraphNode* node, const Material_ptr& matInstance) {
    switch (node->getNode().type()) {
        case SceneNodeType::TYPE_TRANSFORM:
        {
            if (BitCompare(node->componentMask(), ComponentType::SPOT_LIGHT) ||
                BitCompare(node->componentMask(), ComponentType::POINT_LIGHT) ||
                BitCompare(node->componentMask(), ComponentType::DIRECTIONAL_LIGHT) ||
                BitCompare(node->componentMask(), ComponentType::ENVIRONMENT_PROBE))
            {
                return _renderBins[RenderBinType::RBT_IMPOSTOR];
            }
            /*if (BitCompare(node->componentMask(), ComponentType::PARTICLE_EMITTER_COMPONENT) ||
                BitCompare(node->componentMask(), ComponentType::GRASS_COMPONENT))
            {
                return _renderBins[RenderBinType::RBT_TRANSLUCENT];
            }*/
            return nullptr;
        }

        case SceneNodeType::TYPE_VEGETATION:
        case SceneNodeType::TYPE_PARTICLE_EMITTER:
            return _renderBins[RenderBinType::RBT_TRANSLUCENT];

        case SceneNodeType::TYPE_SKY:
            return _renderBins[RenderBinType::RBT_SKY];

        case SceneNodeType::TYPE_WATER:
        case SceneNodeType::TYPE_INFINITEPLANE:
            return _renderBins[RenderBinType::RBT_TERRAIN_AUX];

        // Water is also opaque as refraction and reflection are separate textures
        // We may want to break this stuff up into mesh rendering components and not care about specifics anymore (i.e. just material checks)
        //case SceneNodeType::TYPE_WATER:
        case SceneNodeType::TYPE_OBJECT3D: {
            if (node->getNode().type() == SceneNodeType::TYPE_OBJECT3D) {
                switch (node->getNode<Object3D>().getObjectType()) {
                    case ObjectType::TERRAIN:
                        return _renderBins[RenderBinType::RBT_TERRAIN];

                    case ObjectType::DECAL:
                        return _renderBins[RenderBinType::RBT_TRANSLUCENT];
                    default: break;
                }
            }
            // Check if the object has a material with transparency/translucency
            if (matInstance != nullptr && matInstance->hasTransparency()) {
                // Add it to the appropriate bin if so ...
                return _renderBins[RenderBinType::RBT_TRANSLUCENT];
            }

            //... else add it to the general geometry bin
            return _renderBins[RenderBinType::RBT_OPAQUE];
        }
        default:
        case SceneNodeType::COUNT:
        case SceneNodeType::TYPE_TRIGGER: break;
    }
    return nullptr;
}

void RenderQueue::addNodeToQueue(const SceneGraphNode* sgn, const RenderStagePass stagePass, const F32 minDistToCameraSq, const RenderBinType targetBinType) {
    RenderingComponent* const renderingCmp = sgn->get<RenderingComponent>();
    // We need a rendering component to render the node
    assert(renderingCmp != nullptr);
    const RenderPackage& pkg = renderingCmp->getDrawPackage(stagePass);
    if (!pkg.empty()) {
        RenderBin* rb = getBinForNode(sgn, renderingCmp->getMaterialInstance());
        assert(rb != nullptr);

        if (targetBinType._value == RenderBinType::RBT_COUNT || rb->getType() == targetBinType) {
            rb->addNodeToBin(sgn, pkg, stagePass, minDistToCameraSq);
        }
    }
}

void RenderQueue::populateRenderQueues(const RenderStagePass stagePass, const std::pair<RenderBinType, bool> binAndFlag, RenderQueuePackages& queueInOut) {
    OPTICK_EVENT();
    auto [binType, includeBin] = binAndFlag;

    if (binType._value == RenderBinType::RBT_COUNT) {
        if (!includeBin) {
            // Why are we allowed to exclude everything? idk.
            return;
        }

        for (RenderBin* renderBin : _renderBins) {
            renderBin->populateRenderQueue(stagePass, queueInOut);
        }
    } else {
        // Everything except the specified type or just the specified type
        for (RenderBin* renderBin : _renderBins) {
            if (renderBin->getType() == binType == includeBin) {
                renderBin->populateRenderQueue(stagePass, queueInOut);
            }
        }
    }
}

void RenderQueue::postRender(const SceneRenderState& renderState, const RenderStagePass stagePass, GFX::CommandBuffer& bufferInOut) {
    for (RenderBin* renderBin : _renderBins) {
        renderBin->postRender(renderState, stagePass, bufferInOut);
    }
}

void RenderQueue::sort(RenderStagePass stagePass, const RenderBinType targetBinType, const RenderingOrder renderOrder) {

    OPTICK_EVENT();
    // How many elements should a render bin contain before we decide that sorting should happen on a separate thread
    constexpr U16 threadBias = 64;
    
    if (targetBinType._value != RenderBinType::RBT_COUNT)
    {
        const RenderingOrder sortOrder = renderOrder == RenderingOrder::COUNT ? getSortOrder(stagePass, targetBinType) : renderOrder;
        _renderBins[to_base(targetBinType._value)]->sort(stagePass._stage, sortOrder);
    }
    else
    {
        TaskPool& pool = parent().platformContext().taskPool(TaskPoolType::HIGH_PRIORITY);
        Task* sortTask = CreateTask(pool, DELEGATE<void, Task&>());
        for (RenderBin* renderBin : _renderBins) {
            if (renderBin->getBinSize(stagePass._stage) > threadBias) {
                const RenderingOrder sortOrder = renderOrder == RenderingOrder::COUNT ? getSortOrder(stagePass, renderBin->getType()) : renderOrder;
                Start(*CreateTask(pool,
                                    sortTask,
                                    [renderBin, sortOrder, stagePass](const Task& /*parentTask*/) {
                                        renderBin->sort(stagePass._stage, sortOrder);
                                    }));
            }
        }

        Start(*sortTask);

        for (RenderBin* renderBin : _renderBins) {
            if (renderBin->getBinSize(stagePass._stage) <= threadBias) {
                const RenderingOrder sortOrder = renderOrder == RenderingOrder::COUNT ? getSortOrder(stagePass, renderBin->getType()) : renderOrder;
                renderBin->sort(stagePass._stage, sortOrder);
            }
        }

        Wait(*sortTask);
    }
}

void RenderQueue::refresh(const RenderStage stage, const RenderBinType targetBinType) {
    if (targetBinType._value == RenderBinType::RBT_COUNT) {
        for (RenderBin* renderBin : _renderBins) {
            renderBin->refresh(stage);
        }
    } else {
        for (RenderBin* renderBin : _renderBins) {
            if (renderBin->getType() == targetBinType) {
                renderBin->refresh(stage);
            }
        }
    }
}

U16 RenderQueue::getSortedQueues(const RenderStage stage, const vectorEASTL<RenderBinType>& binTypes, RenderBin::SortedQueues& queuesOut) const {
    OPTICK_EVENT();
    U16 countOut = 0u;

    if (binTypes.empty()) {
        for (const RenderBin* renderBin : _renderBins) {
            RenderBin::SortedQueue& nodes = queuesOut[renderBin->getType()];
            countOut += renderBin->getSortedNodes(stage, nodes);
        }
    } else {
        for (const RenderBinType type : binTypes) {
            const RenderBin* renderBin = _renderBins[type];
            RenderBin::SortedQueue& nodes = queuesOut[type];
            countOut += renderBin->getSortedNodes(stage, nodes);
        }
    }
    return countOut;
}

};