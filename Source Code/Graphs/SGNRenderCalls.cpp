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
        sgn->addBoundingBox(s.second->getBoundingBoxTransformed(), s.second->getNode()->getType());
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
}

void SceneGraphNode::updateBoundingBoxTransform(const mat4<F32>& transform){
    if(getParent()) {//< root
        //Transform the bounding box if we have a new transform
        WriteLock w_lock(_queryLock);
        //if(_boundingBox.Transform(_initialBoundingBox, transform)){
        _boundingBox.Transform(_initialBoundingBox, transform);
    }
    
    //Update the bounding sphere
    _boundingSphere.fromBoundingBox(_boundingBox);
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
        assert(it.second);
        it.second->sceneUpdate(deltaTime, sceneState);
    }
    _elapsedTime += deltaTime;
    
    FOR_EACH(NodeComponents::value_type& it, _components){
        if (it.second){
            it.second->update(deltaTime);
        }
    }

    if(_transform)    _transform->update(deltaTime);
    if (_node)         {
        _node->sceneUpdate(deltaTime, this, sceneState);
        Material* mat = _node->getMaterial();
        if (mat) mat->update(deltaTime);
    }
    if(_shouldDelete) GET_ACTIVE_SCENEGRAPH()->addToDeletionQueue(this);
}

bool SceneGraphNode::onDraw(RenderStage renderStage){
    if (_drawReset[renderStage]){
        _drawReset[renderStage] = false;
        if (_node && !bitCompare(DEPTH_STAGE, renderStage))
            _node->drawReset(this);
    }
    FOR_EACH(NodeComponents::value_type& it, _components){
        if (it.second)
            it.second->onDraw(renderStage);
    }
    //Call any pre-draw operations on the SceneNode (refresh VB, update materials, etc)
    if (_node){
        Material* mat = _node->getMaterial();
        if (mat){
            if (mat->computeShader(false, renderStage)){
                scheduleDrawReset(renderStage); //reset animation on next draw call
                return false;
            }
        }
        if(!_node->onDraw(renderStage))
            return false;
    }

    return true;
}

void SceneGraphNode::postDraw(RenderStage renderStage){
    // Perform any post draw operations regardless of the draw state
    if (_node) _node->postDraw(renderStage);
}

