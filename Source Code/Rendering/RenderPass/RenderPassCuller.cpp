#include "Headers/RenderPassCuller.h"
#include "Headers/RenderQueue.h"

#include "Scenes/Headers/SceneState.h"
#include "Graphs/Headers/SceneGraphNode.h"
#include "Platform/Video/Headers/GFXDevice.h"

namespace Divide {

RenderPassCuller::RenderPassCuller() {
    _visibleNodes.reserve(Config::MAX_VISIBLE_NODES);
    _visibleNodesSorted = false;
}

RenderPassCuller::~RenderPassCuller() {
    _visibleNodes.clear();
}

void RenderPassCuller::sortVisibleNodes() {
    if (_visibleNodesSorted) {
        return;
    }

    std::sort(std::begin(_visibleNodes), std::end(_visibleNodes),
            [](const SceneGraphNode* a, const SceneGraphNode* b) {
                RenderingComponent* renderableA =
                    a->getComponent<RenderingComponent>();
                RenderingComponent* renderableB =
                    b->getComponent<RenderingComponent>();
                return renderableA->drawOrder() < renderableB->drawOrder();
            });

    _visibleNodesSorted = true;
}

/// This method performs the visibility check on the given node and all of it's
/// children and adds them to the RenderQueue
void RenderPassCuller::cullSceneGraph(
    SceneGraphNode& currentNode,
    SceneState& sceneState,
    const std::function<bool(SceneGraphNode*)>& cullingFunction) {
    bool renderingLocked = RenderPassManager::getInstance().isLocked();

    if (!_visibleNodes.empty()) {
        if (renderingLocked &&
            !RenderPassManager::getInstance().isResetQueued()) {
            sortVisibleNodes();
            GFX_DEVICE.buildDrawCommands(_visibleNodes,
                                         sceneState.getRenderState(),
                                         false);
            return;
        }
        refreshNodeList();
    }

    cullSceneGraphCPU(currentNode, sceneState.getRenderState(),
                      cullingFunction);

    const vec3<F32>& eyePos =
        sceneState.getRenderState().getCameraConst().getEye();
    for (SceneGraphNode* node : _visibleNodes) {
        RenderQueue::getInstance().addNodeToQueue(*node, eyePos);
    }

    RenderQueue::getInstance().sort(GFX_DEVICE.getRenderStage());

    cullSceneGraphGPU(sceneState, cullingFunction);

    sortVisibleNodes();
    GFX_DEVICE.buildDrawCommands(_visibleNodes, sceneState.getRenderState(),
                                 true);

    if (!renderingLocked) {
        refreshNodeList();
    }
}

void RenderPassCuller::cullSceneGraphCPU(
    SceneGraphNode& currentNode,
    SceneRenderState& sceneRenderState,
    const std::function<bool(SceneGraphNode*)>& cullingFunction) {
    // No point in updating visual information if the scene disabled object
    // rendering
    // or rendering of their bounding boxes
    if (sceneRenderState.objectState() ==
        SceneRenderState::ObjectRenderState::NO_DRAW) {
        return;
    }
    RenderStage currentStage = GFX_DEVICE.getRenderStage();

    // Bounding Boxes should be updated, so we can early cull now.
    bool skipChildren = false;

    if (currentNode.getParent()) {
        currentNode.inView(false);
        // Skip all of this for inactive nodes.
        if (currentNode.isActive()) {
            SceneNode* node = currentNode.getNode();
            // If this node isn't render-disabled, check if it is visible
            // Skip expensive frustum culling if we shouldn't draw the node in
            // the first place
            if (!node->renderState().getDrawState(currentStage)) {
                // If the current SceneGraphNode isn't visible, it's children
                // aren't visible as well
                skipChildren = true;
            } else {
                RenderingComponent* renderingCmp =
                    currentNode.getComponent<RenderingComponent>();
                if (currentStage != RenderStage::SHADOW_STAGE ||
                    (currentStage == RenderStage::SHADOW_STAGE &&
                     (renderingCmp ? renderingCmp->castsShadows() : false))) {
                    // Perform visibility test on current node
                    if (node->isInView(sceneRenderState, currentNode,
                                       currentStage == RenderStage::SHADOW_STAGE
                                           ? false
                                           : true)) {
                        // If the current node is visible, add it to the render
                        // queue (if it passes our custom culling function)
                        if (!cullingFunction(&currentNode)) {
                            _visibleNodes.push_back(&currentNode);
                        }
                        currentNode.inView(true);
                    }
                }
            }
        }
    }
    // If we don't need to skip child testing
    if (!skipChildren) {
        for (SceneGraphNode::NodeChildren::value_type& it :
             currentNode.getChildren()) {
            cullSceneGraphCPU(*it.second, sceneRenderState, cullingFunction);
        }
    }
}

void RenderPassCuller::cullSceneGraphGPU(
    SceneState& sceneState,
    const std::function<bool(SceneGraphNode*)>& cullingFunction) {
    /* http://http.developer.nvidia.com/GPUGems2/gpugems2_chapter06.html
    TraversalStack.Push(hierarchy.Root);

    while (!TraversalStack.Empty() || !QueryQueue.Empty()) {
        //--PART 1: process finished occlusion queries
        while (!QueryQueue.Empty() &&
               (ResultAvailable(QueryQueue.Front()) ||
                TraversalStack.Empty())) {
            node = QueryQueue.Dequeue();
            // wait if result not available
            visiblePixels = GetOcclusionQueryResult(node);
            if (visiblePixels > VisibilityThreshold) {
                PullUpVisibility(node);
                TraverseNode(node);
            }
        }

        //--PART 2: hierarchical traversal
        if (!TraversalStack.Empty()) {
            node = TraversalStack.Pop();
            if (InsideViewFrustum(node)) {
                // identify previously visible nodes
                wasVisible = node.visible and (node.lastVisited == frameID - 1);
                // identify nodes that we cannot skip queries for
                leafOrWasInvisible = !wasVisible || IsLeaf(node);
                // reset node's visibility classification
                node.visible = false;
                // update node's visited flag
                node.lastVisited = frameID;
                // skip testing previously visible interior nodes
                if (leafOrWasInvisible) {
                    IssueOcclusionQuery(node);
                    QueryQueue.Enqueue(node);
                }

               // always traverse a node if it was visible
                if (wasVisible)
                    TraverseNode(node);
            }
        }
    }

    TraverseNode(node) {
        if (IsLeaf(node))     Render(node);
        else                  TraversalStack.PushChildren(node);
    }

    PullUpVisibility(node) {
        while (!node.visible) {
            node.visible = true;
            node = node.parent;
        }
    }*/
}

void RenderPassCuller::refreshNodeList() {
    _visibleNodes.resize(0);
    _visibleNodes.reserve(Config::MAX_VISIBLE_NODES);
    _visibleNodesSorted = false;
    RenderPassManager::getInstance().isResetQueued(false);
}

void RenderPassCuller::refresh() {
    refreshNodeList();
    RenderQueue::getInstance().refresh(true);
}
};