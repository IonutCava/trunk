#include "Headers/RenderPassCuller.h"
#include "Headers/RenderQueue.h"

#include "Scenes/Headers/SceneState.h"
#include "Graphs/Headers/SceneGraphNode.h"
#include "Hardware/Video/Headers/GFXDevice.h"
#include "Geometry/Material/Headers/Material.h"

RenderPassCuller::RenderPassCuller()
{
    _visibleNodes.reserve(250);
}

RenderPassCuller::~RenderPassCuller()
{
    _visibleNodes.clear();
}

/// This method performs the visibility check on the given node and all of it's children and adds them to the RenderQueue
void RenderPassCuller::cullSceneGraph(SceneGraphNode* const currentNode, SceneState& sceneState) {
    bool renderingLocked = RenderPassManager::getInstance().isLocked();
    if(!_visibleNodes.empty()){
        if(renderingLocked) return;
        else refreshNodeList();
    }

    assert(_visibleNodes.empty());
    cullSceneGraphCPU(currentNode, sceneState.getRenderState());
    for_each(SceneGraphNode* node, _visibleNodes){
        RenderQueue::getInstance().addNodeToQueue(node);
    }
    cullSceneGraphGPU(sceneState);

    if(!renderingLocked) refreshNodeList();
}

void RenderPassCuller::cullSceneGraphCPU(SceneGraphNode* const currentNode, SceneRenderState& sceneRenderState) {
    //No point in updating visual information if the scene disabled object rendering
    //or rendering of their bounding boxes
    if(!sceneRenderState.drawObjects() && !sceneRenderState.drawBBox())
        return;

    //Bounding Boxes should be updated, so we can early cull now.
    bool skipChildren = false;
    //Skip all of this for inactive nodes.
    if(currentNode->isActive() && currentNode->getParent()) {
        SceneNode* node = currentNode->getSceneNode();

        //If this node isn't render-disabled, check if it is visible
        //Skip expensive frustum culling if we shouldn't draw the node in the first place
        if(!node->getSceneNodeRenderState().getDrawState()){
             //If the current SceneGraphNode isn't visible, it's children aren't visible as well
            skipChildren = true;
        }else{
            RenderStage currentStage = GFX_DEVICE.getRenderStage();

            if(currentStage != SHADOW_STAGE || (currentStage == SHADOW_STAGE && currentNode->getCastsShadows())){
                //Perform visibility test on current node
                if(node->isInView(currentNode->getBoundingBox(), currentNode->getBoundingSphere())){
                    //If the current node is visible, add it to the render queue
                    _visibleNodes.push_back(currentNode);
                }
            }
        }
    }

    //If we don't need to skip child testing
    if(!skipChildren){
        for_each(SceneGraphNode::NodeChildren::value_type& it, currentNode->getChildren()) {
            cullSceneGraphCPU(it.second, sceneRenderState);
        }
    }
}

void RenderPassCuller::cullSceneGraphGPU(SceneState& sceneState) {

}

void RenderPassCuller::refreshNodeList() {
    _visibleNodes.resize(0);
    _visibleNodes.reserve(250);
}

void RenderPassCuller::refresh() {
    refreshNodeList();
    RenderQueue::getInstance().refresh(true);
}