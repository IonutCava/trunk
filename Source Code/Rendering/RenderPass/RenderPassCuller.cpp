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
    static const U32 g_nodesPerCullingPartition = 16u;

    // Return true if the node type is capable of generating draw commands
    FORCE_INLINE bool generatesDrawCommands(SceneNodeType nodeType, ObjectType objType) {
        return nodeType != SceneNodeType::TYPE_ROOT &&
               nodeType != SceneNodeType::TYPE_TRANSFORM &&
               nodeType != SceneNodeType::TYPE_TRIGGER &&
               nodeType != SceneNodeType::TYPE_EMPTY &&
               objType._value != ObjectType::MESH;
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

        isTransformNodeOut = snType == SceneNodeType::TYPE_EMPTY ||
                             snType == SceneNodeType::TYPE_TRANSFORM ||
                             objectType._value == ObjectType::MESH;

        if (generatesDrawCommands(snType, objectType)) {
            // only checks nodes and can return true for a shadow stage
            return stage == RenderStage::SHADOW && doesNotCastShadows(stage, node, snType, objectType);
        }

        return true;
    }
};

std::mutex RenderPassCuller::s_nodeListContainerMutex;
MemoryPool<NodeListContainer, NodeListContainerSizeInBytes> RenderPassCuller::s_nodeListContainer;

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
            
        vectorEASTL<VisibleNodeList> nodes(rootChildren.size());
        parallel_for(*params._context,
                     [this, &rootChildren, &nodeParams, &nodes](const Task& parentTask, U32 start, U32 end) {
                        for (U32 i = start; i < end; ++i) {
                            frustumCullNode(parentTask, *rootChildren[i], nodeParams, nodes[i]);
                        }
                     },
                     to_U32(rootChildren.size()),
                     g_nodesPerCullingPartition,
                     nodeParams._threaded ? TaskPriority::DONT_CARE : TaskPriority::REALTIME,
                     false, //Wait for all subtasks to finish! This means that the subtasks can run without waiting as the parent task should keep count of all running child taks
                     true,
                     "Frustum cull task");
        
        for (const VisibleNodeList& nodeListEntry : nodes) {
            nodeCache.insert(eastl::end(nodeCache), eastl::cbegin(nodeListEntry), eastl::cend(nodeListEntry));
        }
    }

    return nodeCache;
}

/// This method performs the visibility check on the given node and all of its children and adds them to the RenderQueue
void RenderPassCuller::frustumCullNode(const Task& task, SceneGraphNode& currentNode, const NodeCullParams& params, VisibleNodeList& nodes) const {
    // Early out for inactive nodes
    if (!currentNode.isActive()) {
        return;
    }

    bool isTransformNode = false;
    Frustum::FrustCollision collisionResult = Frustum::FrustCollision::FRUSTUM_OUT;

    // If it fails the culling test, stop
    if (shouldCullNode(params._stage, currentNode, isTransformNode)) {
        if (isTransformNode) {
            collisionResult = Frustum::FrustCollision::FRUSTUM_INTERSECT;
        } else {
            return;
        }
    }

    F32 distanceSqToCamera = 0.0f;

    // Internal node cull (check against camera frustum and all that ...)
    bool isVisible = isTransformNode || !currentNode.cullNode(params, collisionResult, distanceSqToCamera);

    if (isVisible && !StopRequested(task)) {
        if (!isTransformNode) {
            nodes.emplace_back(VisibleNode{ distanceSqToCamera, &currentNode });
        }
        // Parent node intersects the view, so check children
        if (collisionResult == Frustum::FrustCollision::FRUSTUM_INTERSECT) {

            // A very good use-case for ranges :-<
            vectorEASTL<SceneGraphNode*> children = currentNode.getChildrenLocked();
            NodeListContainer* nodesTemp = s_nodeListContainer.newElement(s_nodeListContainerMutex);
            nodesTemp->resize(children.size());

            parallel_for(*task._parentPool,
                         [this, &children, &params, &nodesTemp](const Task & parentTask, U32 start, U32 end) {
                             for (U32 i = start; i < end; ++i) {
                                frustumCullNode(parentTask, *children[i], params, (*nodesTemp)[i]);
                             }
                         },
                         to_U32(children.size()),
                         g_nodesPerCullingPartition * 2,
                         params._threaded ? TaskPriority::DONT_CARE : TaskPriority::REALTIME,
                         false,
                         true,
                         "Frustum cull node task");
            for (const VisibleNodeList& nodeListEntry : *nodesTemp) {
                nodes.insert(eastl::end(nodes), eastl::cbegin(nodeListEntry), eastl::cend(nodeListEntry));
            }
            s_nodeListContainer.deleteElement(s_nodeListContainerMutex, nodesTemp);
        } else {
            // All nodes are in view entirely
            addAllChildren(currentNode, params, nodes);
        }
    }
}

void RenderPassCuller::addAllChildren(SceneGraphNode& currentNode, const NodeCullParams& params, VisibleNodeList& nodes) const {
    bool castsShadows = currentNode.get<RenderingComponent>()->renderOptionEnabled(RenderingComponent::RenderOptions::CAST_SHADOWS);

    vectorEASTL<SceneGraphNode*> children = currentNode.getChildrenLocked();
    for (SceneGraphNode* child : children) {
        if (child->isActive() && (params._stage != RenderStage::SHADOW || castsShadows || child->visibilityLocked())) {
            bool isTransformNode = false;
            bool shouldCull = shouldCullNode(params._stage, *child, isTransformNode) && !isTransformNode;
            if (!shouldCull) {
                F32 distanceSqToCamera = std::numeric_limits<F32>::max();
                if (!child->preCullNode(params, distanceSqToCamera)) {
                    nodes.emplace_back(VisibleNode{ distanceSqToCamera, child });
                    addAllChildren(*child, params, nodes);
                }
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
        if (!node->cullNode(params, collisionResult, distanceSqToCamera)) {
            ret.emplace_back(VisibleNode{ distanceSqToCamera, node });
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
        ret.emplace_back(VisibleNode{ distanceSqToCamera, node });
    }
    return ret;
}
};