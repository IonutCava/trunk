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

    // Return true if the node can't be drawn but contains command generating children but 
    bool isParentNode(const VisibleNode& node) {
        const SceneGraphNode* sgn = node._node;
        const SceneNode& sceneNode = sgn->getNode();
        if (sceneNode.type() == SceneNodeType::TYPE_OBJECT3D) {
            return sgn->getNode<Object3D>().getObjectType()._value == ObjectType::MESH;
        }
        return false;
    }
};

RenderPassCuller::RenderPassCuller() {
    for (VisibleNodeList& cache : _visibleNodes) {
        cache.reserve(Config::MAX_VISIBLE_NODES);
    }
}

RenderPassCuller::~RenderPassCuller() {
    clear();
}

void RenderPassCuller::clear() {
    for (VisibleNodeList& cache : _visibleNodes) {
        cache.clear();
    }
}

VisibleNodeList& RenderPassCuller::getNodeCache(RenderStage stage) {
    return _visibleNodes[to_U32(stage)];
}

const VisibleNodeList& RenderPassCuller::getNodeCache(RenderStage stage) const {
    return _visibleNodes[to_U32(stage)];
}

bool RenderPassCuller::wasNodeInView(I64 GUID, RenderStage stage) const {
    const VisibleNodeList& cache = getNodeCache(stage);

    VisibleNodeList::const_iterator it;
    it = eastl::find_if(eastl::cbegin(cache), eastl::cend(cache),
        [GUID](VisibleNode node) {
            const SceneGraphNode* nodePtr = node._node;
            return (nodePtr != nullptr && nodePtr->getGUID() == GUID);
        });

    return it != eastl::cend(cache);
}

VisibleNodeList& RenderPassCuller::frustumCull(const CullParams& params)
{
    RenderStage stage = params._stage;
    VisibleNodeList& nodeCache = getNodeCache(stage);
    if (!params._context || !params._sceneGraph || !params._camera || !params._sceneState) {
        return nodeCache;
    }

    nodeCache.clear();
    if (params._sceneState->renderState().isEnabledOption(SceneRenderState::RenderOptions::RENDER_GEOMETRY)) {
        NodeCullParams nodeParams = {};
        nodeParams._currentCamera = params._camera;
        nodeParams._lodThresholds = params._sceneState == nullptr ? vec4<U16>() : params._sceneState->renderState().lodThresholds();
        nodeParams._cullMaxDistanceSq = std::min(params._visibilityDistanceSq, SQUARED(params._camera->getZPlanes().y));
        nodeParams._minLoD = params._minLoD;
        nodeParams._minExtents = params._minExtents;
        nodeParams._threaded = params._threaded;
        nodeParams._stage = stage;

        _cullingFunction[to_U32(stage)] = params._cullFunction;

        // Snapshot current child list. We shouldn't be modifying child list mid-frame anyway!
        vectorEASTL<SceneGraphNode*> rootChildren = params._sceneGraph->getRoot().getChildrenLocked();

        vectorEASTL<VisibleNodeList> nodes(rootChildren.size());
        parallel_for(*params._context,
                     [this, &rootChildren, &nodeParams, &nodes](const Task& parentTask, U32 start, U32 end) {
                        for (U32 i = start; i < end; ++i) {
                            frustumCullNode(parentTask, *rootChildren[i], nodeParams, true, nodes[i]);
                        }
                     },
                     to_U32(rootChildren.size()),
                     g_nodesPerCullingPartition,
                     nodeParams._threaded ? TaskPriority::DONT_CARE : TaskPriority::REALTIME,
                     false, //Wait for all subtasks to finish! This means that the subtasks can run without waiting as the parent task should keep count of all running child taks
                     true);
        
        for (const VisibleNodeList& nodeListEntry : nodes) {
            nodeCache.insert(eastl::end(nodeCache), eastl::cbegin(nodeListEntry), eastl::cend(nodeListEntry));
        }
    }

    // Some nodes don't need to be rendered because they are an aggregate of their children (e.g. Mesh <-> SubMesh)
    nodeCache.erase(eastl::remove_if(eastl::begin(nodeCache),
                                     eastl::end(nodeCache),
                                         [](const VisibleNode& node) -> bool {
                                             return isParentNode(node);
                                         }),
                    eastl::end(nodeCache));

    return nodeCache;
}

/// This method performs the visibility check on the given node and all of its
/// children and adds them to the RenderQueue
void RenderPassCuller::frustumCullNode(const Task& task,
                                       const SceneGraphNode& currentNode,
                                       const NodeCullParams& params,
                                       bool clearList,
                                       VisibleNodeList& nodes) const
{
    if (clearList) {
        nodes.resize(0);
    }
    // Early out for inactive nodes
    if (!currentNode.isActive()) {
        return;
    }
    // If it fails the culling test, stop
    if (_cullingFunction[to_U32(params._stage)](currentNode, currentNode.getNode())) {
        return;
    }

    F32 distanceSqToCamera = 0.0f;
    Frustum::FrustCollision collisionResult = Frustum::FrustCollision::FRUSTUM_OUT;

    // Internal node cull (check against camera frustum and all that ...)
    bool isVisible = !currentNode.cullNode(params, collisionResult, distanceSqToCamera);

    if (isVisible && !StopRequested(task)) {
        nodes.emplace_back(VisibleNode{ distanceSqToCamera, &currentNode });

        // Parent node intersects the view, so check children
        if (collisionResult == Frustum::FrustCollision::FRUSTUM_INTERSECT) {

            vectorEASTL<SceneGraphNode*> children = currentNode.getChildrenLocked();
            vectorEASTL<VisibleNodeList> nodesTemp(children.size());
            parallel_for(*task._parentPool,
                         [this, &children, &params, &nodesTemp](const Task & parentTask, U32 start, U32 end) {
                             for (U32 i = start; i < end; ++i) {
                                frustumCullNode(parentTask, *children[i], params, false, nodesTemp[i]);
                             }
                         },
                         to_U32(children.size()),
                         g_nodesPerCullingPartition * 2,
                         params._threaded ? TaskPriority::DONT_CARE : TaskPriority::REALTIME,
                         false,
                         true);
            for (const VisibleNodeList& nodeListEntry : nodesTemp) {
                nodes.insert(eastl::end(nodes), eastl::cbegin(nodeListEntry), eastl::cend(nodeListEntry));
            }
        } else {
            // All nodes are in view entirely
            addAllChildren(currentNode, params, nodes);
        }
    }
}

void RenderPassCuller::addAllChildren(const SceneGraphNode& currentNode, const NodeCullParams& params, VisibleNodeList& nodes) const {
    bool castsShadows = currentNode.get<RenderingComponent>()->renderOptionEnabled(RenderingComponent::RenderOptions::CAST_SHADOWS);

    vectorEASTL<SceneGraphNode*> children = currentNode.getChildrenLocked();
    for (SceneGraphNode* child : children) {
        if (params._stage != RenderStage::SHADOW || castsShadows || child->visibilityLocked()) {
            if (child->isActive() && !_cullingFunction[to_U32(params._stage)](*child, child->getNode())) {
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