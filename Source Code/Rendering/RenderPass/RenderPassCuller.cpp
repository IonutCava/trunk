#include "stdafx.h"

#include "config.h"

#include "Headers/RenderPassCuller.h"

#include "Core/Headers/EngineTaskPool.h"
#include "Scenes/Headers/SceneState.h"
#include "Graphs/Headers/SceneGraph.h"
#include "Rendering/Camera/Headers/Camera.h"
#include "Platform/Video/Headers/GFXDevice.h"
#include "Geometry/Shapes/Headers/Mesh.h"
#include "ECS/Components/Headers/BoundsComponent.h"
#include "ECS/Components/Headers/RenderingComponent.h"

namespace Divide {

namespace {
    constexpr U32 g_nodesPerCullingPartition = 16u;
    //Worst case scenario: one per each node
    std::array<NodeListContainer, Config::MAX_VISIBLE_NODES> g_tempContainers = {};
    std::array<std::atomic_bool, Config::MAX_VISIBLE_NODES> g_freeList;
    std::atomic_bool g_freeListInitialized = false;
    std::mutex g_tempContainersLock;

    // This would be messy to return as a pair (std::reference_wrapper and the like)
    NodeListContainer& GetAvailableContainer(U32& idxOut) {
        idxOut = 0;
        if (g_freeListInitialized.load()) {
            while(true) {
                bool expected = true;
                if (g_freeList[idxOut].compare_exchange_strong(expected, false)) {
                    return g_tempContainers[idxOut];
                }
                (++idxOut) %= Config::MAX_VISIBLE_NODES;
                if (Config::Build::IS_DEBUG_BUILD && idxOut == 0) {
                    DIVIDE_ASSERT(false, "RenderPassCuller::GetAvailableContainer looped! Please resize container array");
                }
            }
        } else {
            UniqueLock w_lock(g_tempContainersLock);
            // Check again after lock
            if (!g_freeListInitialized.load()) {
                for (auto& x : g_freeList) {
                    std::atomic_init(&x, true);
                }
                g_freeListInitialized = true;
                g_freeList[0] = false;
                g_freeListInitialized.store(true);
            }
            return GetAvailableContainer(idxOut);
        }
    };

    void FreeContainer(NodeListContainer& container, const U32 idx) {
        assert(!g_freeList[idx].load());
        // Don't use clear(). Keep memory allocated
        g_tempContainers[idx].resize(0); 
        g_freeList[idx].store(true);
    }

    FORCE_INLINE bool isTransformNode(SceneNodeType nodeType, ObjectType objType) {
        return nodeType == SceneNodeType::TYPE_EMPTY ||
               nodeType == SceneNodeType::TYPE_TRANSFORM ||
               objType._value == ObjectType::MESH;
    }

    // Return true if the node type is capable of generating draw commands
    FORCE_INLINE bool generatesDrawCommands(SceneNodeType nodeType, ObjectType objType, bool &isTransformNodeOut) {
        if (nodeType == SceneNodeType::TYPE_ROOT && nodeType == SceneNodeType::TYPE_TRIGGER) {
            isTransformNodeOut = false;
            return false;
        }
        isTransformNodeOut = isTransformNode(nodeType, objType);
        return !isTransformNodeOut;
    }

    // Return true if this node should be removed from a shadow pass
    bool doesNotCastShadows(RenderStage stage, const SceneGraphNode& node, SceneNodeType sceneNodeType, ObjectType objType) {
        if (sceneNodeType == SceneNodeType::TYPE_SKY ||
            sceneNodeType == SceneNodeType::TYPE_WATER ||
            sceneNodeType == SceneNodeType::TYPE_INFINITEPLANE ||
            objType._value == ObjectType::DECAL)
        {
            return true;
        }

        RenderingComponent* rComp = node.get<RenderingComponent>();
        assert(rComp != nullptr);
        return !rComp->renderOptionEnabled(RenderingComponent::RenderOptions::CAST_SHADOWS);
    }

    FORCE_INLINE bool shouldCullNode(RenderStage stage, const SceneGraphNode& node, bool& isTransformNodeOut) {
        const SceneNode& sceneNode = node.getNode();
        const SceneNodeType snType = sceneNode.type();

        ObjectType objectType = ObjectType::COUNT;
        if (snType == SceneNodeType::TYPE_OBJECT3D) {
            objectType = static_cast<const Object3D&>(sceneNode).getObjectType();
        }

        if (generatesDrawCommands(snType, objectType, isTransformNodeOut)) {
            // only checks nodes and can return true for a shadow stage
            return stage == RenderStage::SHADOW && doesNotCastShadows(stage, node, snType, objectType);
        }

        return true;
    }
};

void RenderPassCuller::clear() {
    for (VisibleNodeList& cache : _visibleNodes) {
        cache.clear();
    }
}

VisibleNodeList& RenderPassCuller::frustumCull(const CullParams& params)
{
    RenderStage stage = params._stage;
    VisibleNodeList& nodeCache = getNodeCache(stage);
    if (!params._context || !params._sceneGraph || !params._camera || !params._sceneState) {
        return nodeCache;
    }

    nodeCache.resize(0);
    if (params._sceneState->renderState().isEnabledOption(SceneRenderState::RenderOptions::RENDER_GEOMETRY)) {
        NodeCullParams nodeParams = {};
        nodeParams._currentCamera = params._camera;
        nodeParams._lodThresholds =  params._sceneState->renderState().lodThresholds();
        nodeParams._cullMaxDistanceSq = std::min(params._visibilityDistanceSq, SQUARED(params._camera->getZPlanes().y));
        nodeParams._minLoD = params._minLoD;
        nodeParams._minExtents = params._minExtents;
        nodeParams._threaded = params._threaded;
        nodeParams._stage = stage;

        // Snapshot current child list. We shouldn't be modifying child list mid-frame anyway, but it's worth it to be careful!
        vectorEASTL<SceneGraphNode*> rootChildren = params._sceneGraph->getRoot().getChildrenLocked();
        nodeCache.reserve(rootChildren.size());
            
        U32 containerIdx = 0;
        NodeListContainer& tempContainer = GetAvailableContainer(containerIdx);
        tempContainer.resize(rootChildren.size());

        parallel_for(*params._context,
                     [this, &rootChildren, &nodeParams, &tempContainer](const Task& parentTask, U32 start, U32 end) {
                        for (U32 i = start; i < end; ++i) {
                            auto& temp = tempContainer[i];
                            temp.resize(0);
                            frustumCullNode(parentTask, *rootChildren[i], nodeParams, temp);
                        }
                     },
                     to_U32(rootChildren.size()),
                     g_nodesPerCullingPartition,
                     nodeParams._threaded ? TaskPriority::DONT_CARE : TaskPriority::REALTIME,
                     false, //Wait for all subtasks to finish! This means that the subtasks can run without waiting as the parent task should keep count of all running child taks
                     true,
                     "Frustum cull task");
        
        for (const VisibleNodeList& nodeListEntry : tempContainer) {
            nodeCache.insert(eastl::end(nodeCache), eastl::cbegin(nodeListEntry), eastl::cend(nodeListEntry));
        }
        FreeContainer(tempContainer, containerIdx);
    }

    return nodeCache;
}

/// This method performs the visibility check on the given node and all of its children and adds them to the RenderQueue
void RenderPassCuller::frustumCullNode(const Task& task, SceneGraphNode& currentNode, const NodeCullParams& params, VisibleNodeList& nodes) const {
    // Early out for inactive nodes
    if (currentNode.hasFlag(SceneGraphNode::Flags::ACTIVE)) {

        Frustum::FrustCollision collisionResult = Frustum::FrustCollision::FRUSTUM_OUT;

        // If it fails the culling test, stop
        bool isTransformNode = false;
        if (shouldCullNode(params._stage, currentNode, isTransformNode)) {
            if (isTransformNode) {
                collisionResult = Frustum::FrustCollision::FRUSTUM_INTERSECT;
            } else {
                return;
            }
        }

        // Internal node cull (check against camera frustum and all that ...)
        F32 distanceSqToCamera = 0.0f;
        bool isVisible = isTransformNode || !Attorney::SceneGraphNodeRenderPassCuller::cullNode(currentNode, params, collisionResult, distanceSqToCamera);

        if (isVisible && !StopRequested(task)) {
            if (!isTransformNode) {
                nodes.emplace_back(&currentNode, distanceSqToCamera);
            }
            // Parent node intersects the view, so check children
            if (collisionResult == Frustum::FrustCollision::FRUSTUM_INTERSECT) {

                // A very good use-case for ranges :-<
                vectorEASTL<SceneGraphNode*> children = currentNode.getChildrenLocked();

                U32 containerIdx = 0;
                NodeListContainer& tempContainer = GetAvailableContainer(containerIdx);
                tempContainer.resize(children.size());
                parallel_for(*task._parentPool,
                             [this, &children, &params, &tempContainer](const Task & parentTask, U32 start, U32 end) {
                                 for (U32 i = start; i < end; ++i) {
                                    frustumCullNode(parentTask, *children[i], params, tempContainer[i]);
                                 }
                             },
                             to_U32(children.size()),
                             g_nodesPerCullingPartition * 2,
                             params._threaded ? TaskPriority::DONT_CARE : TaskPriority::REALTIME,
                             false,
                             true,
                             "Frustum cull node task");
                for (const VisibleNodeList& nodeListEntry : tempContainer) {
                    nodes.insert(eastl::end(nodes), eastl::cbegin(nodeListEntry), eastl::cend(nodeListEntry));
                }
                FreeContainer(tempContainer, containerIdx);
            } else {
                // All nodes are in view entirely
                addAllChildren(currentNode, params, nodes);
            }
        }
    }
}

void RenderPassCuller::addAllChildren(SceneGraphNode& currentNode, const NodeCullParams& params, VisibleNodeList& nodes) const {
    bool castsShadows = currentNode.get<RenderingComponent>()->renderOptionEnabled(RenderingComponent::RenderOptions::CAST_SHADOWS);

    vectorEASTL<SceneGraphNode*> children = currentNode.getChildrenLocked();
    for (SceneGraphNode* child : children) {
        if (child->hasFlag(SceneGraphNode::Flags::ACTIVE) &&
            (params._stage != RenderStage::SHADOW || castsShadows || child->hasFlag(SceneGraphNode::Flags::VISIBILITY_LOCKED)))
        {
            bool isTransformNode = false;
            bool shouldCull = shouldCullNode(params._stage, *child, isTransformNode);
            if (!shouldCull) {
                F32 distanceSqToCamera = std::numeric_limits<F32>::max();
                if (!Attorney::SceneGraphNodeRenderPassCuller::preCullNode(*child, params, distanceSqToCamera)) {
                    nodes.emplace_back(child, distanceSqToCamera);
                    addAllChildren(*child, params, nodes);
                }
            } else if (isTransformNode) {
                addAllChildren(*child, params, nodes);
            }
        }
    }
}

VisibleNodeList RenderPassCuller::frustumCull(const NodeCullParams& params, const vectorEASTL<SceneGraphNode*>& nodes) const {
    VisibleNodeList ret = {};
    ret.reserve(nodes.size());

    F32 distanceSqToCamera = std::numeric_limits<F32>::max();
    Frustum::FrustCollision collisionResult = Frustum::FrustCollision::FRUSTUM_OUT;
    for (SceneGraphNode* node : nodes) {
        // Internal node cull (check against camera frustum and all that ...)
        if (!Attorney::SceneGraphNodeRenderPassCuller::cullNode(*node, params, collisionResult, distanceSqToCamera)) {
            ret.emplace_back(node, distanceSqToCamera);
        }
    }
    return ret;
}

VisibleNodeList RenderPassCuller::toVisibleNodes(const Camera& camera, const vectorEASTL<SceneGraphNode*>& nodes) const {
    VisibleNodeList ret = {};
    ret.reserve(nodes.size());

    const vec3<F32> cameraEye = camera.getEye();
    for (SceneGraphNode* node : nodes) {
        F32 distanceSqToCamera = std::numeric_limits<F32>::max();
        BoundsComponent* bComp = node->get<BoundsComponent>();
        if (bComp != nullptr) {
            distanceSqToCamera = bComp->getBoundingSphere().getCenter().distanceSquared(cameraEye);
        }
        ret.emplace_back(node, distanceSqToCamera);
    }
    return ret;
}
};