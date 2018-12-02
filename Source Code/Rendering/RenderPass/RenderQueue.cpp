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

RenderQueue::RenderQueue(Kernel& parent)
    : KernelComponent(parent)
{
    _renderBins.fill(nullptr);
    _activeBins.reserve(RenderBinType::_size_constant);
}

RenderQueue::~RenderQueue()
{
    for (RenderBin* bin : _renderBins) {
        MemoryManager::DELETE(bin);
    }
    _renderBins.fill(nullptr);
    _activeBins.clear();
}

U16 RenderQueue::getRenderQueueStackSize(RenderStage stage) const {
    U16 temp = 0;
    for (RenderBin* bin : _activeBins) {
        temp += bin->getBinSize(stage);
    }
    return temp;
}

RenderingOrder::List RenderQueue::getSortOrder(RenderStagePass stagePass, RenderBinType rbType) {
    RenderingOrder::List sortOrder = RenderingOrder::List::COUNT;
    switch (rbType) {
        case RenderBinType::RBT_OPAQUE: {
            // Opaque items should be rendered front to back in depth passes for early-Z reasons
            sortOrder = !stagePass.isDepthPass() ? RenderingOrder::List::BY_STATE
                                                 : RenderingOrder::List::FRONT_TO_BACK;
        } break;
        case RenderBinType::RBT_SKY: {
            sortOrder = RenderingOrder::List::NONE;
        } break;
        case RenderBinType::RBT_IMPOSTOR:
        case RenderBinType::RBT_TERRAIN:
        case RenderBinType::RBT_DECAL: {
            // No need to sort decals in depth passes as they're small on screen and processed post-opaque pass
            sortOrder = !stagePass.isDepthPass() ? RenderingOrder::List::FRONT_TO_BACK
                                                 : RenderingOrder::List::NONE;
        } break;
        case RenderBinType::RBT_TRANSLUCENT: {
            // Translucent items should be rendered by material in depth passes to avoid useless material switches
            // Small cost for bypassing early-Z checks, but translucent items should be in the minority on the screen anyway
            // and the Opaque pass should have finished by now
            /*sortOrder = !stagePass.isDepthPass()  ? RenderingOrder::List::BACK_TO_FRONT
                                                    : RenderingOrder::List::BY_STATE;*/


            // We are using weighted blended OIT. State is fine (and faster)
            sortOrder = RenderingOrder::List::BY_STATE;
        } break;
        default: {
            Console::errorfn(Locale::get(_ID("ERROR_INVALID_RENDER_BIN_CREATION")));
        } break;
    };
    
    return sortOrder;
}

RenderBin* RenderQueue::getOrCreateBin(RenderBinType rbType) {
    RenderBin* temp = getBin(rbType);
    if (temp != nullptr) {
        return temp;
    }

    temp = MemoryManager_NEW RenderBin(rbType);
    // Bins are sorted by their type
    _renderBins[rbType._to_integral()] = temp;
    
    _activeBins.resize(0);
    std::back_insert_iterator<vector<RenderBin*>> back_it(_activeBins);
    auto const usedPredicate = [](RenderBin* ptr) { return ptr != nullptr; };
    std::copy_if(std::begin(_renderBins), std::end(_renderBins), back_it, usedPredicate);

    return temp;
}

RenderBin* RenderQueue::getBinForNode(const SceneGraphNode& node, const Material_ptr& matInstance) {
    assert(node.getNode() != nullptr && matInstance != nullptr);

    switch (node.getNode()->type()) {
        case SceneNodeType::TYPE_EMPTY:
        {
            if (BitCompare(node.componentMask(), ComponentType::SPOT_LIGHT) ||
                BitCompare(node.componentMask(), ComponentType::POINT_LIGHT) ||
                BitCompare(node.componentMask(), ComponentType::DIRECTIONAL_LIGHT))
            {
                return getOrCreateBin(RenderBinType::RBT_IMPOSTOR);
            }
            /*if (BitCompare(node.componentMask(), ComponentType::PARTICLE_EMITTER_COMPONENT) ||
                BitCompare(node.componentMask(), ComponentType::GRASS_COMPONENT))
            {
                return getOrCreateBin(RenderBinType::RBT_TRANSLUCENT);
            }*/
            return nullptr;
        }
        case SceneNodeType::TYPE_VEGETATION_GRASS:
        case SceneNodeType::TYPE_PARTICLE_EMITTER:
            return getOrCreateBin(RenderBinType::RBT_TRANSLUCENT);

        case SceneNodeType::TYPE_SKY:
            return getOrCreateBin(RenderBinType::RBT_SKY);

        case SceneNodeType::TYPE_INFINITEPLANE:
            return getOrCreateBin(RenderBinType::RBT_TERRAIN);

        // Water is also opaque as refraction and reflection are separate textures
        // We may want to break this stuff up into mesh rendering components and not care about specifics anymore (i.e. just material checks)
        case SceneNodeType::TYPE_WATER:
        case SceneNodeType::TYPE_OBJECT3D: {
        case SceneNodeType::TYPE_VEGETATION_TREES:
            if (node.getNode()->type() == SceneNodeType::TYPE_OBJECT3D) {
                ObjectType type = static_cast<Object3D*>(node.getNode().get())->getObjectType();
                switch (type) {
                    case ObjectType::TERRAIN:
                        return getOrCreateBin(RenderBinType::RBT_TERRAIN);

                    case ObjectType::DECAL:
                        return getOrCreateBin(RenderBinType::RBT_DECAL);
                }
            }
            // Check if the object has a material with transparency/translucency
            if (matInstance->hasTransparency()) {
                // Add it to the appropriate bin if so ...
                return getOrCreateBin(RenderBinType::RBT_TRANSLUCENT);
            }

            //... else add it to the general geometry bin
            return getOrCreateBin(RenderBinType::RBT_OPAQUE);
        }
    }
    return nullptr;
}

void RenderQueue::addNodeToQueue(const SceneGraphNode& sgn, RenderStagePass stage, const vec3<F32>& eyePos) {
    RenderingComponent* const renderingCmp = sgn.get<RenderingComponent>();
    // We need a rendering component to render the node
    if (renderingCmp != nullptr) {
        RenderBin* rb = getBinForNode(sgn, renderingCmp->getMaterialInstance());
        assert(rb != nullptr);

        rb->addNodeToBin(sgn, stage, eyePos);
    }
}

void RenderQueue::populateRenderQueues(RenderStagePass stagePass, RenderBinType rbType, vectorEASTL<RenderPackage*>& queueInOut) {
    if (rbType._value == RenderBinType::RBT_COUNT) {
        for (RenderBin* renderBin : _activeBins) {
            renderBin->populateRenderQueue(stagePass, queueInOut);
        }
    } else {
        for (RenderBin* renderBin : _activeBins) {
            if (renderBin->getType() == rbType) {
                renderBin->populateRenderQueue(stagePass, queueInOut);
                break;
            }
        }
    }
}

void RenderQueue::postRender(const SceneRenderState& renderState, RenderStagePass stagePass, GFX::CommandBuffer& bufferInOut) {
    for (RenderBin* renderBin : _activeBins) {
        renderBin->postRender(renderState, stagePass, bufferInOut);
    }
}

void RenderQueue::sort(RenderStagePass stagePass) {
    // How many elements should a renderbin contain before we decide that sorting should happen on a separate thread
    static const U16 threadBias = 32;

    TaskPool& pool = parent().platformContext().taskPool();
    TaskHandle sortTask = CreateTask(pool, DELEGATE_CBK<void, const Task&>());
    for (RenderBin* renderBin : _activeBins) {
        if (!renderBin->empty(stagePass._stage)) {
            RenderingOrder::List sortOrder = getSortOrder(stagePass, renderBin->getType());

            if (renderBin->getBinSize(stagePass._stage) > threadBias) {
                CreateTask(pool,
                           &sortTask,
                            [renderBin, sortOrder, stagePass](const Task& parentTask) {
                                renderBin->sort(stagePass._stage, sortOrder, parentTask);
                            }).startTask();
            } else {
                renderBin->sort(stagePass._stage, sortOrder);
            }
        }
    }
    sortTask.startTask().wait();
}

void RenderQueue::refresh(RenderStage stage) {
    for (RenderBin* renderBin : _activeBins) {
        renderBin->refresh(stage);
    }
}

void RenderQueue::getSortedQueues(RenderStage stage, SortedQueues& queuesOut, U16& countOut) const {
    for (RenderBin* renderBin : _activeBins) {
        vectorEASTL<SceneGraphNode*>& nodes = queuesOut[renderBin->getType()._to_integral()];
        renderBin->getSortedNodes(stage, nodes, countOut);
    }
}

};