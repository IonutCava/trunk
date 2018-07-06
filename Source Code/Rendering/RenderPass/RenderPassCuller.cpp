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

RenderPassCuller::VisibleNodeList&
RenderPassCuller::getNodeCache(RenderStage stage) {
    switch (stage) {
        case RenderStage::REFLECTION:
            return _visibleNodes[1];
        case RenderStage::SHADOW:
            return _visibleNodes[2];
        default:
            break;
    }

    return _visibleNodes[0];
}

void RenderPassCuller::frustumCull(SceneGraph& sceneGraph,
                                   SceneState& sceneState,
                                   RenderStage stage,
                                   const CullingFunction& cullingFunction)
{
    VisibleNodeList& nodeCache = getNodeCache(stage);
    nodeCache.resize(0);
    if (sceneState.renderState().drawGeometry()) {
        _cullingFunction = cullingFunction;
        SceneRenderState& renderState = sceneState.renderState();
        // No point in updating visual information if the scene disabled object
        // rendering or rendering of their bounding boxes
        SceneGraphNode& root = *sceneGraph.getRoot();
        U32 childCount = root.getChildCount();
        _cullingTasks.resize(0);
        _perThreadNodeList.resize(childCount);
        for (U32 i = 0; i < childCount; ++i) {
            SceneGraphNode& child = root.getChild(i, childCount);
            VisibleNodeList& container = _perThreadNodeList[i];
            container.resize(0);
            _cullingTasks.push_back(std::async(/*std::launch::async | */std::launch::deferred, [&]() {
                frustumCullRecursive(child, stage, renderState, container);
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

bool RenderPassCuller::frustumCullNode(SceneGraphNode& node,
                                       RenderStage currentStage,
                                       SceneRenderState& sceneRenderState) {
    // Skip all of this for inactive nodes.
    if (node.isActive()) {
        // If the current node is visible, add it to the render
        // queue (if it passes our custom culling function)
        if (!node.cullNode(sceneRenderState, currentStage)) {
            return _cullingFunction(node);
        }
    }

    return true;
}

/// This method performs the visibility check on the given node and all of its
/// children and adds them to the RenderQueue
void RenderPassCuller::frustumCullRecursive(SceneGraphNode& currentNode,
                                            RenderStage currentStage,
                                            SceneRenderState& sceneRenderState,
                                            VisibleNodeList& nodes)
{
    // Skip processing children if the parent node isn't visible
    if (!frustumCullNode(currentNode, currentStage, sceneRenderState)) {
        nodes.push_back(currentNode.shared_from_this());
        // Process children if we did not early-out of the culling loop
        U32 childCount = currentNode.getChildCount();
        for (U32 i = 0; i < childCount; ++i) {
            frustumCullRecursive(currentNode.getChild(i, childCount), currentStage, sceneRenderState, nodes);
        }
    }
}

};