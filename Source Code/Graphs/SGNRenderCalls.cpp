#include "Headers/SceneGraphNode.h"
#include "Rendering/RenderPass/Headers/RenderQueue.h"
#include "Managers/Headers/SceneManager.h"
#include "Geometry/Shapes/Headers/SubMesh.h"
#include "Geometry/Shapes/Headers/Mesh.h"

void SceneGraphNode::checkBoundingBoxes(){
	//Update order is very important!
	//Ex: Mesh BB is composed of SubMesh BB's. 
	//Compute from leaf to root to ensure proper calculations
	for_each(NodeChildren::value_type& it, _children){
		it.second->checkBoundingBoxes();
	}
	// don't update root;
	if(!getParent()) return; 

	//Compute the BoundingBox if it isn't already
	if(!getBoundingBox().isComputed()){
		_node->computeBoundingBox(this);
	}
	///Recreate bounding boxes for current frame	
	_node->updateBBatCurrentFrame(this);
}

//This function eats up a lot of processing power
//It compute's the BoundingBoxes of all transformed nodes and updates transforms
void SceneGraphNode::updateTransforms(){
	//update every ten milliseconds - DISABLED. 
	//Better version: move to new thread with DoubleBuffering?
	//if(GETMSTIME() - _updateTimer  < 10) return; 
	//Get our transform and our parent's as well
	Transform* transform = getTransform();
	if(transform && getParent()){
		Transform* parentTransform = getParent()->getTransform();
		if(parentTransform){
			//If we have a transform and a parent's transform 
			//Update the relationship between the two
			transform->setParentMatrix(parentTransform->getMatrix());
		}

		//Transform the bounding box if we have a new transform 
		if(transform->isDirty()){
			_node->updateTransform(this);
			getBoundingBox().Transform(_initialBoundingBox, transform->getGlobalMatrix());
		}
	}

	for_each(NodeChildren::value_type& it, _children){
		it.second->updateTransforms();
	}
	//update every ten milliseconds - DISABLED. 
	//_updateTimer = GETMSTIME();
}

//Another resource hungry subroutine
//After all bounding boxes and transforms have been updated, 
//perform Frustum Culling on the entire scene.
//ToDo: implement an early cull so that we don't check the entire scene -Ionut
//Previous attempts have proven buggy
void SceneGraphNode::updateVisualInformation(){
	//Hold a pointer to the current active scene
	Scene* curentScene = GET_ACTIVE_SCENE();
	//No point in updating visual information if the scene disabled object rendering 
	//or rendering of their bounding boxes
	if(!curentScene->drawObjects() && !curentScene->drawBBox()) return;
	//Bounding Boxes should be updated, so we can early cull now.
	//Early cull switch
	bool _skipChildren = false;
	//Skip all of this for inactive nodes.
	if(_active && getParent()) {
		//Hold a pointer to the scenegraph of the current active scene
		if(!_sceneGraph){
			_sceneGraph = curentScene->getSceneGraph();
		}
		//If this node isn't render-disabled, check if it is visible
		//Skip expensive frustum culling if we shouldn't draw the node in the first place
		if(_node->getDrawState()){ 
			switch(GFX_DEVICE.getRenderStage()){
				default:
				case FINAL_STAGE:
				case DEFERRED_STAGE:{
					//Perform visibility test on current node
					_inView = _node->isInView(true,getBoundingBox());
				} break; 

				case SHADOW_STAGE: {
					_inView = false;
					if(_node->getMaterial()){
						if(_node->getMaterial()->getCastsShadows()){
							_inView = _node->isInView(true,getBoundingBox());
						}
					}
				}break;
			};
		}else{
			//If the current SceneGraphNode isn't visible, it's children aren't visible as well
			_inView = false;
			_skipChildren = true;
		}

		if(_inView){
			//If the current node is visible, add it to the render queue
			RenderQueue::getInstance().addNodeToQueue(this);
		}
	}
	//If we don't need to skip child testing
	if(!_skipChildren){
		for_each(NodeChildren::value_type& it, _children){
			it.second->updateVisualInformation();
		}
	}
}

void SceneGraphNode::sceneUpdate(D32 sceneTime) {
	for_each(NodeChildren::value_type& it, _children){
		it.second->sceneUpdate(sceneTime);
	}
	if(_node){
		_node->sceneUpdate(sceneTime);
	}
	if(_shouldDelete){
		_sceneGraph->addToDeletionQueue(this);
	}
}