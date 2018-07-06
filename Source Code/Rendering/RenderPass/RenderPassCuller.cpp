#include "Headers/RenderPassCuller.h"

#include "Scenes/Headers/SceneState.h"
#include "Graphs/Headers/SceneGraph.h"
#include "Platform/Video/Headers/GFXDevice.h"

namespace Divide {

RenderPassCuller::RenderPassCuller() {
    for (VisibleNodeList& cache : _visibleNodes) {
        cache.reserve(Config::MAX_VISIBLE_NODES);
    }
}

RenderPassCuller::~RenderPassCuller() {
    for (VisibleNodeList& cache : _visibleNodes) {
        cache.clear();
    }
}

U32 RenderPassCuller::stageToCacheIndex(RenderStage stage) const {
    switch (stage) {
        case RenderStage::REFLECTION:  return 1;
        case RenderStage::SHADOW:  return 2;
    };

    return 0;
}

RenderPassCuller::VisibleNodeList&
RenderPassCuller::getNodeCache(RenderStage stage) {
    return _visibleNodes[stageToCacheIndex(stage)];
}

const RenderPassCuller::VisibleNodeList&
RenderPassCuller::getNodeCache(RenderStage stage) const {
    return _visibleNodes[stageToCacheIndex(stage)];
}

bool RenderPassCuller::wasNodeInView(I64 GUID, RenderStage stage) const {
    const VisibleNodeList& cache = getNodeCache(stage);

    VisibleNodeList::const_iterator it;
    it = std::find_if(std::cbegin(cache), std::cend(cache),
        [GUID](SceneGraphNode_wptr node) {
            SceneGraphNode_ptr nodePtr = node.lock();
            return (nodePtr != nullptr && nodePtr->getGUID() == GUID);
        });

    return it != std::cend(cache);
}

void RenderPassCuller::frustumCull(SceneGraph& sceneGraph,
                                   SceneState& sceneState,
                                   RenderStage stage,
                                   bool async,
                                   const CullingFunction& cullingFunction)
{
    VisibleNodeList& nodeCache = getNodeCache(stage);
    nodeCache.resize(0);

    if (sceneState.renderState().drawGeometry()) {
        _cullingFunction = cullingFunction;
        SceneRenderState& renderState = sceneState.renderState();
        // No point in updating visual information if the scene disabled object
        // rendering or rendering of their bounding boxes
        SceneGraphNode& root = sceneGraph.getRoot();
        U32 childCount = root.getChildCount();
        _cullingTasks.resize(0);
        _perThreadNodeList.resize(childCount);
        for (U32 i = 0; i < childCount; ++i) {
            SceneGraphNode& child = root.getChild(i, childCount);
            VisibleNodeList& container = _perThreadNodeList[i];
            container.resize(0);
            _cullingTasks.push_back(std::async(async ? std::launch::async | std::launch::deferred
                                                     : std::launch::deferred,
            [&]() {
                frustumCullNode(child, stage, renderState, container);
            }));
        }

        for (std::future<void>& task : _cullingTasks) {
            task.get();
        }

        for (VisibleNodeList& nodeList : _perThreadNodeList) {
            for (SceneGraphNode_wptr ptr : nodeList) {
                nodeCache.push_back(ptr);
            }
        }
    }
}

/// This method performs the visibility check on the given node and all of its
/// children and adds them to the RenderQueue
void RenderPassCuller::frustumCullNode(SceneGraphNode& currentNode,
                                       RenderStage currentStage,
                                       SceneRenderState& sceneRenderState,
                                       VisibleNodeList& nodes)
{
    Frustum::FrustCollision collisionResult = Frustum::FrustCollision::FRUSTUM_OUT;
    bool isVisible = !(currentStage == RenderStage::SHADOW &&
                       !currentNode.getComponent<RenderingComponent>()->castsShadows());

    isVisible = isVisible &&
                currentNode.isActive() &&
                !_cullingFunction(currentNode) &&
                !currentNode.cullNode(sceneRenderState, collisionResult, currentStage);

    if (isVisible) {
        nodes.push_back(currentNode.shared_from_this());
        U32 childCount = currentNode.getChildCount();
        if (childCount > 0) {
            if (collisionResult == Frustum::FrustCollision::FRUSTUM_INTERSECT) {
                // Parent node intersects the view, so check children
                for (U32 i = 0; i < childCount; ++i) {
                    frustumCullNode(currentNode.getChild(i, childCount),
                                    currentStage,
                                    sceneRenderState,
                                    nodes);
                }
            } else {
                // All nodes are in view entirely
               addAllChildren(currentNode, currentStage, nodes);
            }
        }
    }
}

void RenderPassCuller::addAllChildren(SceneGraphNode& currentNode, RenderStage currentStage, VisibleNodeList& nodes) {
    U32 childCount = currentNode.getChildCount();
    for (U32 i = 0; i < childCount; ++i) {
        SceneGraphNode_ptr child = currentNode.getChild(i, childCount).shared_from_this();
        bool isVisible = !(currentStage == RenderStage::SHADOW &&
                           !currentNode.getComponent<RenderingComponent>()->castsShadows());

        if (isVisible && child->isActive() && !_cullingFunction(*child)) {
            nodes.push_back(child);
            addAllChildren(*child, currentStage, nodes);
        }
    }
}

};