#include "Headers/SceneGraphNode.h"

#include "Scenes/Headers/SceneState.h"
#include "Core/Math/Headers/Transform.h"
#include "Managers/Headers/SceneManager.h"
#include "Geometry/Shapes/Headers/Mesh.h"
#include "Geometry/Shapes/Headers/SubMesh.h"
#include "Geometry/Material/Headers/Material.h"
#include "Rendering/RenderPass/Headers/RenderQueue.h"

bool SceneRoot::computeBoundingBox(SceneGraphNode* const sgn) {
    sgn->getBoundingBox().reset();
    for_each(SceneGraphNode::NodeChildren::value_type& s, sgn->getChildren()){
        sgn->addBoundingBox(s.second->getBoundingBoxTransformed(), s.second->getNode<SceneNode>()->getType());
    }
    sgn->getBoundingBox().setComputed(true);
    return true;
}

///Please call in MAIN THREAD! Nothing is thread safe here (for now) -Ionut
void SceneGraphNode::checkBoundingBoxes(){
    //Update order is very important!
    //Ex: Mesh BB is composed of SubMesh BB's.
    //Compute from leaf to root to ensure proper calculations
    for_each(NodeChildren::value_type& it, _children){
        it.second->checkBoundingBoxes();
    }

    if(_node->getState() != RES_LOADED)
        return;

    //Compute the BoundingBox if it isn't already
    if(!_boundingBox.isComputed()){
        _node->computeBoundingBox(this);
        _boundingSphere.fromBoundingBox(_boundingBox);
        
        if(_parent)
           _parent->getBoundingBox().setComputed(false);
        _updateBB = false;
    }

    //Recreate bounding boxes for current frame
    _node->updateBBatCurrentFrame(this);
}

//This function eats up a lot of processing power
//It computes the BoundingBoxes of all transformed nodes and updates transforms
void SceneGraphNode::updateTransforms(){
    //Better version: move to new thread with DoubleBuffering?
    //Get our transform and our parent's as well
    Transform* transform = getTransform();
    Transform* parentTransform = NULL;
    if(transform){
        parentTransform = (_parent != NULL ? _parent->getTransform() : NULL);
        transform->setParentTransform(parentTransform);
        if(_transform->isDirty() || (parentTransform && parentTransform->isDirty()))
            updateBoundingBoxTransform(_transform->getGlobalMatrix());
    }

    _node->updateTransform(this);

    for_each(NodeChildren::value_type& it, _children){
        it.second->updateTransforms();
    }
}

void SceneGraphNode::updateBoundingBoxTransform(const mat4<F32>& transform){
    if(!getParent())
        return;

    //Transform the bounding box if we have a new transform
    WriteLock w_lock(_queryLock);
    _boundingBox.Transform(_initialBoundingBox,transform);
    //Update the bounding sphere
    _boundingSphere.fromBoundingBox(_boundingBox);
}

void SceneGraphNode::setSelected(bool state) {
    _selected = state;
     for_each(NodeChildren::value_type& it, _children){
        it.second->setSelected(_selected);
    }
}

///Another resource hungry subroutine
///After all bounding boxes and transforms have been updated,
///perform Frustum Culling on the entire scene.
void SceneGraphNode::updateVisualInformation(){
    //No point in updating visual information if the scene disabled object rendering
    //or rendering of their bounding boxes
    if(!_currentSceneState->getRenderState().drawObjects() && !_currentSceneState->getRenderState().drawBBox())
        return;

    //Bounding Boxes should be updated, so we can early cull now.
    bool skipChildren = false;
    //Skip all of this for inactive nodes.
    if(_active && _parent) {
        //If this node isn't render-disabled, check if it is visible
        //Skip expensive frustum culling if we shouldn't draw the node in the first place

        if(_node->getSceneNodeRenderState().getDrawState()){
            switch(GFX_DEVICE.getRenderStage()){
                default: {
                    //Perform visibility test on current node
                    _inView = _node->isInView(getBoundingBox(),getBoundingSphere());
                } break;

                case SHADOW_STAGE: {
                    _inView = false;
                    if(_node->getMaterial()){
                        if(_node->getMaterial()->getCastsShadows()){
                            _inView = _node->isInView(getBoundingBox(),getBoundingSphere());
                        }
                    }
                }break;
            };
        }else{
            //If the current SceneGraphNode isn't visible, it's children aren't visible as well
            _inView = false;
            skipChildren = true;
        }

        if(_inView){
            //If the current node is visible, add it to the render queue
            RenderQueue::getInstance().addNodeToQueue(this);
        }
    }
    //If we don't need to skip child testing
    if(!skipChildren){
        for_each(NodeChildren::value_type& it, _children){
            it.second->updateVisualInformation();
        }
    }
}

void SceneGraphNode::sceneUpdate(const D32 deltaTime, SceneState& sceneState) {
    for_each(NodeChildren::value_type& it, _children){
        it.second->sceneUpdate(deltaTime, sceneState);
    }

    _currentSceneState = &sceneState;

    if(_node)
        _node->sceneUpdate(deltaTime, this, sceneState);

    if(_shouldDelete)
        GET_ACTIVE_SCENEGRAPH()->addToDeletionQueue(this);
}