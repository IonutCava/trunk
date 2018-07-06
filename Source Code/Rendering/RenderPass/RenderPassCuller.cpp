#include "Headers/RenderPassCuller.h"
#include "Headers/RenderQueue.h"

#include "Scenes/Headers/SceneState.h"
#include "Graphs/Headers/SceneGraphNode.h"
#include "Hardware/Video/Headers/GFXDevice.h"
#include "Geometry/Material/Headers/Material.h"

RenderPassCuller::RenderPassCuller()
{
    _visibleNodes.reserve(Config::MAX_VISIBLE_NODES);
}

RenderPassCuller::~RenderPassCuller()
{
    _visibleNodes.clear();
}

/// This method performs the visibility check on the given node and all of it's children and adds them to the RenderQueue
void RenderPassCuller::cullSceneGraph(SceneGraphNode* const currentNode, SceneState& sceneState) {
    bool renderingLocked = RenderPassManager::getInstance().isLocked();

    if(!_visibleNodes.empty()){
        bool refreshQueued = RenderPassManager::getInstance().isResetQueued();
        if(renderingLocked && !refreshQueued) return;
        else refreshNodeList();
    }

    cullSceneGraphCPU(currentNode, sceneState.getRenderState());

    const vec3<F32>& eyePos = sceneState.getRenderState().getCameraConst().getEye();
    for(SceneGraphNode* node : _visibleNodes)
        RenderQueue::getInstance().addNodeToQueue(node, eyePos);
    
    cullSceneGraphGPU(sceneState);

    currentNode->getRoot()->inView(true);

    GFX_DEVICE.processVisibleNodes(_visibleNodes);

    if(!renderingLocked) refreshNodeList();
}

void RenderPassCuller::cullSceneGraphCPU(SceneGraphNode* const currentNode, SceneRenderState& sceneRenderState) {
    //No point in updating visual information if the scene disabled object rendering
    //or rendering of their bounding boxes
    if(!sceneRenderState.drawObjects() && !sceneRenderState.drawBBox())
        return;

    currentNode->inView(false);
    //Bounding Boxes should be updated, so we can early cull now.
    bool skipChildren = false;

    //Skip all of this for inactive nodes.
    if(currentNode->isActive() && currentNode->getParent()) {
        SceneNode* node = currentNode->getNode();
        RenderStage currentStage = GFX_DEVICE.getRenderStage();
        //If this node isn't render-disabled, check if it is visible
        //Skip expensive frustum culling if we shouldn't draw the node in the first place
        if (!node->getSceneNodeRenderState().getDrawState(currentStage)){
             //If the current SceneGraphNode isn't visible, it's children aren't visible as well
            skipChildren = true;
        }else{
            if(currentStage != SHADOW_STAGE || (currentStage == SHADOW_STAGE && currentNode->getCastsShadows())){
                //Perform visibility test on current node
                if (node->isInView(sceneRenderState,
                                   currentNode->getBoundingBoxConst(), 
                                   currentNode->getBoundingSphereConst(), 
                                   currentStage == SHADOW_STAGE ? false : true)){
                    //If the current node is visible, add it to the render queue
                    _visibleNodes.push_back(currentNode);
                    currentNode->inView(true);
                }
            }
        }
    }

    //If we don't need to skip child testing
    if(!skipChildren){
        FOR_EACH(SceneGraphNode::NodeChildren::value_type& it, currentNode->getChildren()) {
            cullSceneGraphCPU(it.second, sceneRenderState);
        }
    }
}

void RenderPassCuller::cullSceneGraphGPU(SceneState& sceneState) {
    /* http://http.developer.nvidia.com/GPUGems2/gpugems2_chapter06.html
    TraversalStack.Push(hierarchy.Root);

    while (!TraversalStack.Empty() || !QueryQueue.Empty()) {
        //--PART 1: process finished occlusion queries
        while (!QueryQueue.Empty() && (ResultAvailable(QueryQueue.Front()) || TraversalStack.Empty())) {
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
    _visibleNodes.clear();
    _visibleNodes.reserve(Config::MAX_VISIBLE_NODES);
    RenderPassManager::getInstance().isResetQueued(false);
}

void RenderPassCuller::refresh() {
    refreshNodeList();
    RenderQueue::getInstance().refresh(true);
}