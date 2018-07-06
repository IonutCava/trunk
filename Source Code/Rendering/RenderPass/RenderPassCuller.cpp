#include "stdafx.h"

#include "config.h"

#include "Headers/RenderPassCuller.h"

#include "Core/Headers/EngineTaskPool.h"
#include "Scenes/Headers/SceneState.h"
#include "Graphs/Headers/SceneGraph.h"
#include "Rendering/Camera/Headers/Camera.h"
#include "Platform/Video/Headers/GFXDevice.h"
#include "Geometry/Shapes/Headers/Mesh.h"

namespace Divide {

namespace {
    static const U32 g_nodesPerCullingPartition = 16u;

    // Return true if the node can't be drawn but contains command generating children but 
    bool isParentNode(const RenderPassCuller::VisibleNode& node) {
        const SceneGraphNode* sgn = node._node;
        const SceneNode_ptr& sceneNode = sgn->getNode();
        if (sceneNode->type() == SceneNodeType::TYPE_OBJECT3D) {
            return sgn->getNode<Object3D>()->getObjectType() == Object3D::ObjectType::MESH;
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

RenderPassCuller::VisibleNodeList&
RenderPassCuller::getNodeCache(RenderStage stage) {
    return _visibleNodes[to_U32(stage)];
}

const RenderPassCuller::VisibleNodeList&
RenderPassCuller::getNodeCache(RenderStage stage) const {
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

RenderPassCuller::VisibleNodeList& RenderPassCuller::frustumCull(const CullParams& params)
{
    RenderStage stage = params._stage;
    VisibleNodeList& nodeCache = getNodeCache(stage);
    if (!params._context || !params._sceneGraph || !params._camera || !params._sceneState) {
        return nodeCache;
    }

    nodeCache.clear();
    if (params._sceneState->renderState().isEnabledOption(SceneRenderState::RenderOptions::RENDER_GEOMETRY)) {
        const Camera& camera = *params._camera;

        F32 cullMaxDistanceSq = std::min(params._visibilityDistanceSq, SQUARED(camera.getZPlanes().y));

        _cullingFunction[to_U32(stage)] = params._cullFunction;

        const SceneGraphNode& root = params._sceneGraph->getRoot();

        U32 childCount = root.getChildCount();
        vectorEASTL<VisibleNodeList> nodes(childCount);

        auto cullIterFunction = [this, &root, &camera, &nodes, &stage, cullMaxDistanceSq](const Task& parentTask, U32 start, U32 end) {
            auto perChildCull = [this, &parentTask, &camera, &nodes, start, &stage, cullMaxDistanceSq](const SceneGraphNode& child, I32 i) {
                frustumCullNode(parentTask, child, camera, stage, cullMaxDistanceSq, nodes[i], true);
            };
            root.forEachChild(perChildCull, start, end);
        };

        parallel_for(*params._context,
                     cullIterFunction,
                     childCount,
                     g_nodesPerCullingPartition);

        
        for (const VisibleNodeList& nodeListEntry : nodes) {
            nodeCache.insert(eastl::end(nodeCache), eastl::cbegin(nodeListEntry), eastl::cend(nodeListEntry));
        }
    }

    // Some nodes don't need to be rendered because they are an aggregate of their children (e.g. Mesh <-> SubMesh)
    nodeCache.erase(eastl::remove_if(eastl::begin(nodeCache),
                                     eastl::end(nodeCache),
                                         [](const RenderPassCuller::VisibleNode& node) -> bool {
                                             return isParentNode(node);
                                         }),
                    eastl::end(nodeCache));

    return nodeCache;
}

/// This method performs the visibility check on the given node and all of its
/// children and adds them to the RenderQueue
void RenderPassCuller::frustumCullNode(const Task& parentTask,
                                       const SceneGraphNode& currentNode,
                                       const Camera& currentCamera,
                                       RenderStage stage,
                                       F32 cullMaxDistanceSq,
                                       VisibleNodeList& nodes,
                                       bool clearList) const
{
    if (clearList) {
        nodes.resize(0);
    }
    // Early out for inactive nodes
    if (!currentNode.isActive()) {
        return;
    }
    // If it fails thhe culling test, stop
    if (_cullingFunction[to_U32(stage)](currentNode)) {
        return;
    }

    F32 distanceSqToCamera = 0.0f;
    Frustum::FrustCollision collisionResult = Frustum::FrustCollision::FRUSTUM_OUT;

    // Internal node cull (check against camera frustum and all that ...)
    bool isVisible = !currentNode.cullNode(currentCamera, cullMaxDistanceSq, stage, collisionResult, distanceSqToCamera);

    if (isVisible && !StopRequested(&parentTask)) {
        nodes.emplace_back(VisibleNode{ distanceSqToCamera, &currentNode });
        if (collisionResult == Frustum::FrustCollision::FRUSTUM_INTERSECT) {
            // Parent node intersects the view, so check children
            auto childCull = [this, &parentTask, &currentCamera, &nodes, &stage, cullMaxDistanceSq](const SceneGraphNode& child) {
                frustumCullNode(parentTask, child, currentCamera, stage, cullMaxDistanceSq, nodes, false);
            };

            currentNode.forEachChild(childCull);
        } else {
            // All nodes are in view entirely
            addAllChildren(currentNode, stage, currentCamera.getEye(), nodes);
        }
    }
}

void RenderPassCuller::addAllChildren(const SceneGraphNode& currentNode, RenderStage stage, const vec3<F32>& cameraEye, VisibleNodeList& nodes) const {
    currentNode.forEachChild([this, &currentNode, &stage, &cameraEye, &nodes](const SceneGraphNode& child) {
        if (!(stage == RenderStage::SHADOW && !currentNode.get<RenderingComponent>()->renderOptionEnabled(RenderingComponent::RenderOptions::CAST_SHADOWS))) {
            if (child.isActive() && !_cullingFunction[to_U32(stage)](child)) {
                F32 distanceSqToCamera = std::numeric_limits<F32>::max();
                BoundsComponent* bComp = child.get<BoundsComponent>();
                if (bComp != nullptr) {
                    distanceSqToCamera = bComp->getBoundingSphere().getCenter().distanceSquared(cameraEye);
                }
                nodes.emplace_back(VisibleNode{ distanceSqToCamera, &child });
                addAllChildren(child, stage, cameraEye, nodes);
            }
        }
    });
}

RenderPassCuller::VisibleNodeList RenderPassCuller::frustumCull(const Camera& camera, F32 maxDistanceSq, RenderStage stage, const vectorEASTL<SceneGraphNode*>& nodes) const {
    RenderPassCuller::VisibleNodeList ret;

    F32 distanceSqToCamera = std::numeric_limits<F32>::max();
    Frustum::FrustCollision collisionResult = Frustum::FrustCollision::FRUSTUM_OUT;
    for (SceneGraphNode* node : nodes) {
        // Internal node cull (check against camera frustum and all that ...)
        if (!node->cullNode(camera, maxDistanceSq, stage, collisionResult, distanceSqToCamera)) {
            ret.emplace_back(VisibleNode{ distanceSqToCamera, node });
        }
    }
    return ret;
}

RenderPassCuller::VisibleNodeList RenderPassCuller::toVisibleNodes(const Camera& camera, const vectorEASTL<SceneGraphNode*>& nodes) const {
    RenderPassCuller::VisibleNodeList ret;

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