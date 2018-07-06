#include "Headers/SceneGraphNode.h"

#include "Core/Math/Headers/Transform.h"
#include "Managers/Headers/SceneManager.h"
#include "Geometry/Shapes/Headers/Mesh.h"
#include "Geometry/Shapes/Headers/SubMesh.h"
#include "Geometry/Material/Headers/Material.h"
#include "Rendering/RenderPass/Headers/RenderQueue.h"

///Please call in MAIN THREAD! Nothing is thread safe here (for now) -Ionut
void SceneGraphNode::checkBoundingBoxes(){
	//Update order is very important!
	//Ex: Mesh BB is composed of SubMesh BB's.
	//Compute from leaf to root to ensure proper calculations
	for_each(NodeChildren::value_type& it, _children){
		it.second->checkBoundingBoxes();
	}
	// don't update root;
	if(!getParent()) return;

	if(_node->getState() != RES_LOADED) return;
	//Compute the BoundingBox if it isn't already
    if(!_boundingBox.isComputed()){
        _node->computeBoundingBox(this);
        _boundingSphere.fromBoundingBox(_boundingBox);
    }
    if(_node->getDrawState()) {
        getRoot()->addBoundingBox(_boundingBox,_node->getType());
    }
	///Recreate bounding boxes for current frame
	_node->updateBBatCurrentFrame(this);
}

//This function eats up a lot of processing power
//It compute's the BoundingBoxes of all transformed nodes and updates transforms
void SceneGraphNode::updateTransforms(){
	//Better version: move to new thread with DoubleBuffering?
	//Get our transform and our parent's as well
	Transform* transform = getTransform();
	if(transform && _parent && _parent->getTransform()){
		Transform* parentTransform = _parent->getTransform();
		//If we have a transform and a parent's transform
		//Update the relationship between the two
        if(transform->getParentMatrix() != parentTransform->getMatrix()){
			transform->setParentMatrix(parentTransform->getMatrix());
        }
        updateBoundingBoxTransform();
    }

 	for_each(NodeChildren::value_type& it, _children){
		it.second->updateTransforms();
	}
}

void SceneGraphNode::updateBoundingBoxTransform(){
    //Transform the bounding box if we have a new transform
	if(_transform->isDirty()){
		_node->updateTransform(this);

        WriteLock w_lock(_queryLock);
		_boundingBox.Transform(_initialBoundingBox,_transform->getGlobalMatrix());
        //Update the bounding sphere
        _boundingSphere.fromBoundingBox(_boundingBox);
    }
}

///Another resource hungry subroutine
///After all bounding boxes and transforms have been updated,
///perform Frustum Culling on the entire scene.
void SceneGraphNode::updateVisualInformation(){
	//Hold a pointer to the current active scene
	Scene* curentScene = GET_ACTIVE_SCENE();
	//No point in updating visual information if the scene disabled object rendering
	//or rendering of their bounding boxes
	if(!curentScene->renderState()->drawObjects() && !curentScene->renderState()->drawBBox()) return;
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
					_inView = _node->isInView(true,getBoundingBox(),getBoundingSphere());
				} break;

				case SHADOW_STAGE: {
					_inView = false;
					if(_node->getMaterial()){
						if(_node->getMaterial()->getCastsShadows()){
							_inView = _node->isInView(true,getBoundingBox(),getBoundingSphere());
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

void SceneGraphNode::sceneUpdate(const U32 sceneTime) {
	for_each(NodeChildren::value_type& it, _children){
		it.second->sceneUpdate(sceneTime);
	}

	if(_node){
		_node->sceneUpdate(sceneTime,this);
	}
	if(_shouldDelete){
		GET_ACTIVE_SCENE()->getSceneGraph()->addToDeletionQueue(this);
	}
}