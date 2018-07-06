#include "Headers/RenderPassCuller.h"

#include "Scenes/Headers/SceneState.h"
#include "Graphs/Headers/SceneGraphNode.h"
#include "Platform/Video/Headers/GFXDevice.h"

namespace Divide {

RenderPassCuller::RenderPassCuller() {
    for (VisibleNodeCache& cache : _visibleNodes) {
        cache._sorted = false;
        cache._visibleNodes.reserve(Config::MAX_VISIBLE_NODES);
    }
}

RenderPassCuller::~RenderPassCuller() {
    for (VisibleNodeCache& cache : _visibleNodes) {
        cache._visibleNodes.clear();
    }
}

void RenderPassCuller::refresh() {
    for (VisibleNodeCache& cache : _visibleNodes) {
        cache._visibleNodes.resize(0);
        cache._sorted = false;
    }
}

RenderPassCuller::VisibleNodeCache& RenderPassCuller::getNodeCache(RenderStage stage) {
    RenderPassCuller::VisibleNodeCache& nodes = _visibleNodes[0];
    switch (stage) {
        default:
            nodes = _visibleNodes[0];
            break;
        case RenderStage::REFLECTION:
            nodes = _visibleNodes[1];
            break;
        case RenderStage::SHADOW:
            nodes = _visibleNodes[2];
            break;
    }

    return nodes;
}

/// This method performs the visibility check on the given node and all of its
/// children and adds them to the RenderQueue
RenderPassCuller::VisibleNodeCache& RenderPassCuller::frustumCull(
    SceneGraphNode& currentNode, SceneState& sceneState,
    const std::function<bool(const SceneGraphNode&)>& cullingFunction) 
{
    VisibleNodeCache& nodes = getNodeCache(GFX_DEVICE.getRenderStage());

    if (nodes._visibleNodes.empty()) {
        // No point in updating visual information if the scene disabled object
        // rendering or rendering of their bounding boxes
        if (sceneState.renderState().drawGeometry()) {
            cullSceneGraphCPU(nodes, currentNode, sceneState.renderState(),
                              cullingFunction);
            nodes._sorted = false;
        }
        nodes._locked = false;
    } else {
        nodes._locked = true;
    }
    return nodes;
}

void RenderPassCuller::cullSceneGraphCPU(VisibleNodeCache& nodes,
                                         SceneGraphNode& currentNode,
                                         SceneRenderState& sceneRenderState,
                                         const std::function<bool(const SceneGraphNode&)>& cullingFunction) {
    if (currentNode.getParent().lock()) {
        currentNode.setInView(false);
        // Skip all of this for inactive nodes.
        if (currentNode.isActive()) {
            // Perform visibility test on current node
            if (currentNode.canDraw(sceneRenderState,
                                    GFX_DEVICE.getRenderStage())) {
                // If the current node is visible, add it to the render
                // queue (if it passes our custom culling function)
                if (!cullingFunction(currentNode)) {
                    nodes._visibleNodes.push_back(RenderableNode(currentNode));
                    currentNode.setInView(true);
                }
            } else {
                // Skip processing children if the parent node isn't visible
                return;
            }
        }
    }

    // Process children if we did not early-out of the culling loop
    for (SceneGraphNode::NodeChildren::value_type& it : currentNode.getChildren()) {
        cullSceneGraphCPU(nodes, *it.second, sceneRenderState, cullingFunction);
    }
}

RenderPassCuller::VisibleNodeCache& RenderPassCuller::occlusionCull(
    RenderPassCuller::VisibleNodeCache& inputNodes) {
    return inputNodes;
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

};