#include "stdafx.h"

#include "Headers/RenderQueue.h"

#include "Core/Headers/Kernel.h"
#include "Core/Headers/Console.h"
#include "Core/Headers/TaskPool.h"
#include "Core/Headers/Application.h"
#include "Core/Headers/PlatformContext.h"
#include "Utility/Headers/Localization.h"
#include "Graphs/Headers/SceneGraphNode.h"
#include "Geometry/Shapes/Headers/Object3D.h"
#include "Geometry/Material/Headers/Material.h"
#include "Managers/Headers/RenderPassManager.h"
#include "ECS/Components/Headers/RenderingComponent.h"

namespace Divide {

namespace {
    constexpr bool g_useDiffSortForPrePass = false;
};

RenderQueue::RenderQueue(Kernel& parent)
    : KernelComponent(parent),
      _renderBins{nullptr}
{
    for (RenderBinType rbType : RenderBinType::_values()) {
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

    _renderBins.fill(nullptr);
}

U16 RenderQueue::getRenderQueueStackSize(RenderStage stage) const {
    U16 temp = 0;
    for (RenderBin* bin : _renderBins) {
        if (bin != nullptr) {
            temp += bin->getBinSize(stage);
        }
    }
    return temp;
}

RenderingOrder RenderQueue::getSortOrder(RenderStagePass stagePass, RenderBinType rbType) {
    RenderingOrder sortOrder = RenderingOrder::BY_STATE;
    switch (rbType) {
        case RenderBinType::RBT_OPAQUE: {
            if (g_useDiffSortForPrePass) {
                // Opaque items should be rendered front to back in depth passes for early-Z reasons
                sortOrder = stagePass.isDepthPass() ? RenderingOrder::FRONT_TO_BACK
                                                    : RenderingOrder::BY_STATE;
            } else {
                sortOrder = RenderingOrder::BY_STATE;
            }
        } break;
        case RenderBinType::RBT_SKY: {
            sortOrder = RenderingOrder::NONE;
        } break;
        case RenderBinType::RBT_IMPOSTOR:
        case RenderBinType::RBT_TERRAIN: {
            if (g_useDiffSortForPrePass) {
                sortOrder = stagePass.isDepthPass() ? RenderingOrder::NONE
                                                    : RenderingOrder::FRONT_TO_BACK;
            } else {
                sortOrder = RenderingOrder::FRONT_TO_BACK;
            }
        } break;
        case RenderBinType::RBT_TERRAIN_AUX: {
            // Water first, everything else after
            sortOrder = RenderingOrder::WATER_FIRST;
        } break;
        case RenderBinType::RBT_TRANSLUCENT: {
             // We are using weighted blended OIT. State is fine (and faster)
            sortOrder = RenderingOrder::BY_STATE;
        } break;
        default: {
            Console::errorfn(Locale::get(_ID("ERROR_INVALID_RENDER_BIN_CREATION")));
        } break;
    };
    
    return sortOrder;
}

RenderBin* RenderQueue::getBinForNode(const SceneGraphNode& node, const Material_ptr& matInstance) {
    switch (node.getNode().type()) {
        case SceneNodeType::TYPE_EMPTY:
        {
            if (BitCompare(node.componentMask(), ComponentType::SPOT_LIGHT) ||
                BitCompare(node.componentMask(), ComponentType::POINT_LIGHT) ||
                BitCompare(node.componentMask(), ComponentType::DIRECTIONAL_LIGHT))
            {
                return _renderBins[RenderBinType::RBT_IMPOSTOR];
            }
            /*if (BitCompare(node.componentMask(), ComponentType::PARTICLE_EMITTER_COMPONENT) ||
                BitCompare(node.componentMask(), ComponentType::GRASS_COMPONENT))
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
            if (node.getNode().type() == SceneNodeType::TYPE_OBJECT3D) {
                ObjectType type = node.getNode<Object3D>().getObjectType();
                switch (type) {
                    case ObjectType::TERRAIN:
                        return _renderBins[RenderBinType::RBT_TERRAIN];

                    case ObjectType::DECAL:
                        return _renderBins[RenderBinType::RBT_TRANSLUCENT];
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
    }
    return nullptr;
}

void RenderQueue::addNodeToQueue(const SceneGraphNode& sgn, RenderStagePass stage, F32 minDistToCameraSq) {
    RenderingComponent* const renderingCmp = sgn.get<RenderingComponent>();
    // We need a rendering component to render the node
    if (renderingCmp != nullptr) {
        RenderBin* rb = getBinForNode(sgn, renderingCmp->getMaterialInstance());
        assert(rb != nullptr);

        rb->addNodeToBin(sgn, stage, minDistToCameraSq);
    }
}

void RenderQueue::populateRenderQueues(RenderStagePass stagePass, std::pair<RenderBinType, bool> binAndFlag, vectorEASTLFast<RenderPackage*>& queueInOut) {
    OPTICK_EVENT();

    if (binAndFlag.first._value == RenderBinType::RBT_COUNT) {
        if (!binAndFlag.second) {
            // Why are we allowed to exclude everything? idk.
            return;
        }

        for (RenderBin* renderBin : _renderBins) {
            renderBin->populateRenderQueue(stagePass, queueInOut);
        }
    } else {
        // Everything except the specified type or just the specified type
        for (RenderBin* renderBin : _renderBins) {
            if ((renderBin->getType() == binAndFlag.first) == binAndFlag.second) {
                renderBin->populateRenderQueue(stagePass, queueInOut);
            }
        }
    }
}

void RenderQueue::postRender(const SceneRenderState& renderState, RenderStagePass stagePass, GFX::CommandBuffer& bufferInOut) {
    for (RenderBin* renderBin : _renderBins) {
        renderBin->postRender(renderState, stagePass, bufferInOut);
    }
}

void RenderQueue::sort(RenderStagePass stagePass) {

    OPTICK_EVENT();
    // How many elements should a renderbin contain before we decide that sorting should happen on a separate thread
    constexpr U16 threadBias = 64;
    
    TaskPool& pool = parent().platformContext().taskPool(TaskPoolType::HIGH_PRIORITY);
    Task* sortTask = CreateTask(pool, DELEGATE_CBK<void, Task&>(), "Render queue parent sort task");
    for (RenderBin* renderBin : _renderBins) {
        if (renderBin->getBinSize(stagePass._stage) > threadBias) {
            RenderingOrder sortOrder = getSortOrder(stagePass, renderBin->getType());
            Start(*CreateTask(pool,
                                sortTask,
                                [renderBin, sortOrder, stagePass](const Task& parentTask) {
                                    renderBin->sort(stagePass._stage, sortOrder, parentTask);
                                },"Render queue sort task"));
        }
    }

    Start(*sortTask);

    for (RenderBin* renderBin : _renderBins) {
        const U16 size = renderBin->getBinSize(stagePass._stage);
        if (size > 0 && size <= threadBias) {
            renderBin->sort(stagePass._stage, getSortOrder(stagePass, renderBin->getType()));
        }
    }

    Wait(*sortTask);
}

void RenderQueue::refresh(RenderStage stage) {
    for (RenderBin* renderBin : _renderBins) {
        renderBin->refresh(stage);
    }
}

void RenderQueue::getSortedQueues(RenderStagePass stagePass, RenderBin::SortedQueues& queuesOut, U16& countOut) const {
    OPTICK_EVENT();

    if (stagePass._passType == RenderPassType::PRE_PASS) {
        constexpr RenderBinType rbTypes[] = {
            RenderBinType::RBT_OPAQUE,
            RenderBinType::RBT_IMPOSTOR,
            RenderBinType::RBT_TERRAIN,
            RenderBinType::RBT_TERRAIN_AUX,
            RenderBinType::RBT_SKY,
            RenderBinType::RBT_TRANSLUCENT
        };
         
        for (const RenderBinType type : rbTypes) {
            const RenderBin* renderBin = _renderBins[type];
            RenderBin::SortedQueue& nodes = queuesOut[type];
            renderBin->getSortedNodes(stagePass._stage, nodes, countOut);
        }
    } else {
        for (const RenderBin* renderBin : _renderBins) {
            RenderBin::SortedQueue& nodes = queuesOut[renderBin->getType()];
            renderBin->getSortedNodes(stagePass._stage, nodes, countOut);
        }
    }
}

};