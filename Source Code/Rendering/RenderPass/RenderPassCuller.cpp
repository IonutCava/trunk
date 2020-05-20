#include "stdafx.h"

#include "Headers/RenderPassCuller.h"

#include "Core/Headers/PlatformContext.h"
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
    constexpr size_t g_maxParallelCullingRequests = 32u;
    constexpr U32 g_nodesPerCullingPartition = 32u;

    [[nodiscard]] inline bool isTransformNode(SceneNodeType nodeType, ObjectType objType) noexcept {
        return nodeType == SceneNodeType::TYPE_TRANSFORM || 
               nodeType == SceneNodeType::TYPE_TRIGGER || 
               objType._value == ObjectType::MESH;
    }

    // Return true if this node should be removed from a shadow pass
    [[nodiscard]] bool doesNotCastShadows(RenderStage stage, const SceneGraphNode& node, SceneNodeType sceneNodeType, ObjectType objType) {
        if (sceneNodeType == SceneNodeType::TYPE_SKY ||
            sceneNodeType == SceneNodeType::TYPE_WATER ||
            sceneNodeType == SceneNodeType::TYPE_INFINITEPLANE ||
            objType._value == ObjectType::DECAL)
        {
            return true;
        }

        const RenderingComponent* rComp = node.get<RenderingComponent>();
        assert(rComp != nullptr);
        return !rComp->renderOptionEnabled(RenderingComponent::RenderOptions::CAST_SHADOWS);
    }

    [[nodiscard]] inline bool shouldCullNode(RenderStage stage, const SceneGraphNode& node, bool& isTransformNodeOut) {
        const SceneNode& sceneNode = node.getNode();
        const SceneNodeType snType = sceneNode.type();

        ObjectType objectType = ObjectType::COUNT;
        if (snType == SceneNodeType::TYPE_OBJECT3D) {
            objectType = static_cast<const Object3D&>(sceneNode).getObjectType();
        }

        if (node.hasFlag(SceneGraphNode::Flags::VISIBILITY_LOCKED)) {
            return false;
        }

        isTransformNodeOut = isTransformNode(snType, objectType);
        if (!isTransformNodeOut) {
            // only checks nodes and can return true for a shadow stage
            return stage == RenderStage::SHADOW && doesNotCastShadows(stage, node, snType, objectType);
        }

        return true;
    }
};

void VisibleNodeList::append(const VisibleNodeList& other) {
    assert(_index + other._index < _nodes.size());

    std::memcpy(_nodes.data() + _index, other._nodes.data(), other._index * sizeof(VisibleNode));
    _index += other._index;
}

void VisibleNodeList::append(const VisibleNode& node) {
    _nodes[_index.fetch_add(1)] = node;
}

bool RenderPassCuller::onStartup() {
  
    return true;
}

bool RenderPassCuller::onShutdown() {
    return true;
}

void RenderPassCuller::clear() noexcept {
    for (VisibleNodeList& cache : _visibleNodes) {
        cache.reset();
    }
}

VisibleNodeList& RenderPassCuller::frustumCull(const NodeCullParams& params, const SceneGraph& sceneGraph, const SceneState& sceneState, PlatformContext& context)
{
    OPTICK_EVENT();

    const RenderStage stage = params._stage;
    VisibleNodeList& nodeCache = getNodeCache(stage);
    nodeCache.reset();

    if (sceneState.renderState().isEnabledOption(SceneRenderState::RenderOptions::RENDER_GEOMETRY) ||
        sceneState.renderState().isEnabledOption(SceneRenderState::RenderOptions::RENDER_WIREFRAME))
    {
        // Snapshot current child list. We shouldn't be modifying child list mid-frame anyway, but it's worth it to be careful!
        vectorEASTL<SceneGraphNode*> rootChildren = sceneGraph.getRoot().getChildrenLocked();

        ParallelForDescriptor descriptor = {};
        descriptor._iterCount = to_U32(rootChildren.size());
        descriptor._partitionSize = g_nodesPerCullingPartition;
        descriptor._priority = TaskPriority::DONT_CARE;
        descriptor._useCurrentThread = true;
        descriptor._cbk = [&](const Task* parentTask, U32 start, U32 end) {
                            for (U32 i = start; i < end; ++i) {
                                frustumCullNode(parentTask, *rootChildren[i], params, 0u, nodeCache);
                            }
                        };
        parallel_for(context, descriptor);
    }

    return nodeCache;
}

/// This method performs the visibility check on the given node and all of its children and adds them to the RenderQueue
void RenderPassCuller::frustumCullNode(const Task* task, SceneGraphNode& currentNode, const NodeCullParams& params, U8 recursionLevel, VisibleNodeList& nodes) const {
    OPTICK_EVENT();

    // Early out for inactive nodes
    if (currentNode.hasFlag(SceneGraphNode::Flags::ACTIVE)) {

        Frustum::FrustCollision collisionResult = Frustum::FrustCollision::FRUSTUM_OUT;
        const I64 nodeGUID = currentNode.getGUID();
        for (size_t i = 0; i < params._ignoredGUIDS.second; ++i) {
            if (nodeGUID == params._ignoredGUIDS.first[i]) {
                return;
            }
        }

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
        if (isTransformNode || !Attorney::SceneGraphNodeRenderPassCuller::cullNode(currentNode, params, collisionResult, distanceSqToCamera)) {
            if (!isTransformNode) {
                VisibleNode node = {};
                node._node = &currentNode;
                node._distanceToCameraSq = distanceSqToCamera;
                nodes.append(node);
            }
            // Parent node intersects the view, so check children
            if (collisionResult == Frustum::FrustCollision::FRUSTUM_INTERSECT) {

                // A very good use-case for ranges :-<
                vectorEASTL<SceneGraphNode*> children = currentNode.getChildrenLocked();

                ParallelForDescriptor descriptor = {};
                descriptor._iterCount = to_U32(children.size());
                descriptor._partitionSize = g_nodesPerCullingPartition;
                descriptor._priority = recursionLevel < 2 ? TaskPriority::DONT_CARE : TaskPriority::REALTIME;
                descriptor._useCurrentThread = true;
                descriptor._cbk = [&](const Task* parentTask, U32 start, U32 end) {
                                        for (U32 i = start; i < end; ++i) {
                                            frustumCullNode(parentTask, *children[i], params, recursionLevel + 1, nodes);
                                        }
                                    };
                parallel_for(currentNode.context(), descriptor);
            } else {
                // All nodes are in view entirely
                addAllChildren(currentNode, params, nodes);
            }
        }
    }
}

void RenderPassCuller::addAllChildren(const SceneGraphNode& currentNode, const NodeCullParams& params, VisibleNodeList& nodes) const {
    OPTICK_EVENT();

    const vectorEASTL<SceneGraphNode*>& children = currentNode.getChildrenLocked();
    for (SceneGraphNode* child : children) {
        if (!child->hasFlag(SceneGraphNode::Flags::ACTIVE)) {
            continue;
        }
        
        bool isTransformNode = false;
        if (!shouldCullNode(params._stage, *child, isTransformNode)) {
            F32 distanceSqToCamera = std::numeric_limits<F32>::max();
            if (!Attorney::SceneGraphNodeRenderPassCuller::preCullNode(*child, *(child->get<BoundsComponent>()), params, distanceSqToCamera)) {
                VisibleNode node = {};
                node._node = child;
                node._distanceToCameraSq = distanceSqToCamera;
                nodes.append(node);

                addAllChildren(*child, params, nodes);
            }
        } else if (isTransformNode) {
            addAllChildren(*child, params, nodes);
        }
    }
}

void RenderPassCuller::frustumCull(const NodeCullParams& params, const vectorEASTL<SceneGraphNode*>& nodes, VisibleNodeList& nodesOut) const {
    OPTICK_EVENT();

    nodesOut.reset();

    F32 distanceSqToCamera = std::numeric_limits<F32>::max();
    Frustum::FrustCollision collisionResult = Frustum::FrustCollision::FRUSTUM_OUT;
    for (SceneGraphNode* node : nodes) {
        // Internal node cull (check against camera frustum and all that ...)
        if (!Attorney::SceneGraphNodeRenderPassCuller::cullNode(*node, params, collisionResult, distanceSqToCamera)) {
            nodesOut.append({ node, distanceSqToCamera });
        }
    }
}

void RenderPassCuller::toVisibleNodes(const Camera& camera, const vectorEASTL<SceneGraphNode*>& nodes, VisibleNodeList& nodesOut) const {
    OPTICK_EVENT();

    nodesOut.reset();

    const vec3<F32> cameraEye = camera.getEye();
    for (SceneGraphNode* node : nodes) {
        F32 distanceSqToCamera = std::numeric_limits<F32>::max();
        BoundsComponent* bComp = node->get<BoundsComponent>();
        if (bComp != nullptr) {
            distanceSqToCamera = bComp->getBoundingSphere().getCenter().distanceSquared(cameraEye);
        }
        nodesOut.append({ node, distanceSqToCamera });
    }
}
};