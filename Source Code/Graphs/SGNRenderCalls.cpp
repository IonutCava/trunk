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
    FOR_EACH(SceneGraphNode::NodeChildren::value_type& s, sgn->getChildren()){
        sgn->addBoundingBox(s.second->getBoundingBoxTransformed(), s.second->getSceneNode()->getType());
    }
    sgn->getBoundingBox().setComputed(true);
    return true;
}

//This function eats up a lot of processing power
//It computes the BoundingBoxes of all transformed nodes and updates transforms
void SceneGraphNode::updateTransforms(){
    //Better version: move to new thread with DoubleBuffering?
    //Get our transform and our parent's as well
    if(getTransform()){
        if(_transform->isDirty() || _transform->setParentTransform((_parent != nullptr ? _parent->getTransform() : nullptr)))
            updateBoundingBoxTransform(_transform->getGlobalMatrix());
    }

    FOR_EACH(NodeChildren::value_type& it, _children){
        it.second->updateTransforms();
    }
}

///Please call in MAIN THREAD! Nothing is thread safe here (for now) -Ionut
void SceneGraphNode::checkBoundingBoxes(){
    //Update order is very important!
    //Ex: Mesh BB is composed of SubMesh BB's.
    //Compute from leaf to root to ensure proper calculations
    FOR_EACH(NodeChildren::value_type& it, _children){
        it.second->checkBoundingBoxes();
    }

    if(_node->getState() != RES_LOADED)
        return;

    //Compute the BoundingBox if it isn't already
    if(!_boundingBox.isComputed()) { 
        _node->computeBoundingBox(this);
        _boundingBox.setComputed(true);
        _boundingBoxDirty = true;
    }

    if(_boundingBoxDirty && _transform){
        updateBoundingBoxTransform(_transform->getGlobalMatrix());
        if(_parent) _parent->getBoundingBox().setComputed(false);
        _boundingBoxDirty = false;
    }

    //Recreate bounding boxes for current frame
    _node->updateBBatCurrentFrame(this);
}

void SceneGraphNode::updateBoundingBoxTransform(const mat4<F32>& transform){
    if(!getParent())
        return;

    //Transform the bounding box if we have a new transform
    WriteLock w_lock(_queryLock);
    //if(_boundingBox.Transform(_initialBoundingBox, transform)){
    _boundingBox.Transform(_initialBoundingBox, transform);
        //Update the bounding sphere
        _boundingSphere.fromBoundingBox(_boundingBox);
    //}
}

void SceneGraphNode::setInitialBoundingBox(const BoundingBox& initialBoundingBox){
    WriteLock w_lock(_queryLock);

    _initialBoundingBox = initialBoundingBox;
    _initialBoundingBox.setComputed(true);
    _boundingBoxDirty = true;
}

void SceneGraphNode::setSelected(bool state) {
    _selected = state;
     FOR_EACH(NodeChildren::value_type& it, _children){
        it.second->setSelected(_selected);
    }
}

void SceneGraphNode::sceneUpdate(const U64 deltaTime, SceneState& sceneState) {
    FOR_EACH(NodeChildren::value_type& it, _children){
        it.second->sceneUpdate(deltaTime, sceneState);
    }
    _elapsedTime += deltaTime;
    if(_transform)    _transform->update(deltaTime);
    if(_node)         _node->sceneUpdate(deltaTime, this, sceneState);
    if(_shouldDelete) GET_ACTIVE_SCENEGRAPH()->addToDeletionQueue(this);
}