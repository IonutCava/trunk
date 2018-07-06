#include "Headers/SceneGraph.h"
#include "Headers/SceneGraphNode.h"

#include "Core/Math/Headers/Transform.h"
#include "Geometry/Shapes/Headers/Object3D.h"
#include "Geometry/Material/Headers/Material.h"
#include "Environment/Water/Headers/Water.h"
#include "Environment/Terrain/Headers/Terrain.h"
#include "Hardware/Video/Shaders/Headers/ShaderProgram.h"

SceneGraphNode::SceneGraphNode(SceneNode* const node) : _node(node),
												  _parent(NULL),
												  _grandParent(NULL),
												  _transform(NULL),
												  _wasActive(true),
											      _active(true),
												  _noDefaultTransform(false),
												  _inView(true),
												  _sorted(false),
												  _silentDispose(false),
												  _updateBB(true),
												  _shouldDelete(false),
                                                  _isReady(false),
												  _updateTimer(GETMSTIME()),
												  _childQueue(0),
                                                  _bbAddExclusionList(0),
                                                  _usageContext(NODE_DYNAMIC),
                                                  _navigationContext(NODE_IGNORE),
												  _overrideNavMeshDetail(false)
{
	_animationTransforms.clear();
}

///If we are destroyng the current graph node
SceneGraphNode::~SceneGraphNode(){
	//delete children nodes recursively
	for_each(NodeChildren::value_type it, _children){
		SAFE_DELETE(it.second);
	}

	//and delete the transform bound to this node
	SAFE_DELETE(_transform);
	_children.clear();
}

void SceneGraphNode::addBoundingBox(const BoundingBox& bb, const SceneNodeType& type) {
    if(!bitCompare(_bbAddExclusionList, type)){
        _boundingBox.Add(bb);
    }
}

SceneGraphNode*  SceneGraphNode::getRoot() const {
    return _sceneGraph->getRoot();
}

vectorImpl<BoundingBox >&  SceneGraphNode::getBBoxes(vectorImpl<BoundingBox >& boxes ){
	//Unload every sub node recursively
	for_each(NodeChildren::value_type& it, _children){
		it.second->getBBoxes(boxes);
	}
    ReadLock r_lock(_queryLock);
	boxes.push_back(_boundingBox);
	return boxes;
}

///When unloading the current graph node
bool SceneGraphNode::unload(){
	//Unload every sub node recursively
	for_each(NodeChildren::value_type& it, _children){
		it.second->unload();
	}
	//Some debug output ...
	if(!_silentDispose && getParent() && _node){
		PRINT_FN(Locale::get("REMOVE_SCENEGRAPH_NODE"),_node->getName().c_str(), getName().c_str());
	}
	//if not root
	if(getParent()){
		//Remove the SceneNode that we own
		RemoveResource(_node);
	}
	return true;
}

///Prints out the SceneGraph structure to the Console
void SceneGraphNode::print(){
	//Starting from the current node
	SceneGraphNode* parent = this;
	U8 i = 0;
	//Count how deep in the graph we are
	//by counting how many ancestors we have before the "root" node
	while(parent != NULL){
		parent = parent->getParent();
		i++;
	}
	//get out material's name
	Material* mat = _node->getMaterial();

	//Some strings to hold the names of our material and shader
	std::string material("none"),shader("none"),depthShader("none");
	//If we have a material
	if(mat){
		//Get the material's name
		material = mat->getName();
		//If we have a shader
		if(mat->getShaderProgram()){
			//Get the shader's name
			shader = mat->getShaderProgram()->getName();
		}
		if(mat->getShaderProgram(DEPTH_STAGE)){
			//Get the depth shader's name
			depthShader = mat->getShaderProgram(DEPTH_STAGE)->getName();
		}
	}
	//Print our current node's information
		PRINT_FN(Locale::get("PRINT_SCENEGRAPH_NODE"), getName().c_str(),
										               _node->getName().c_str(),
													   material.c_str(),
													   shader.c_str(),
													   depthShader.c_str());
	//Repeat for each child, but prefix it with the appropriate number of dashes
	//Based on our ancestor counting earlier
	for_each(NodeChildren::value_type& it, _children){
		for(U8 j = 0; j < i; j++){
			PRINT_F("-");
		}
		it.second->print();
	}
}

///Change current SceneGraphNode's parent
void SceneGraphNode::setParent(SceneGraphNode* const parent) {
	if(_parent){
		//Remove us from the old parent's children map
		NodeChildren::iterator it = _parent->getChildren().find(getName());
		_parent->getChildren().erase(it);
	}
	//Set the parent pointer to the new parent
	_parent = parent;
	//Add ourselves in the new parent's children map
	//Time to add it to the children map
	std::pair<Unordered_map<std::string, SceneGraphNode*>::iterator, bool > result;
	//Try and add it to the map
	result = _parent->getChildren().insert(std::make_pair(getName(),this));
	//If we had a collision (same name?)
	if(!result.second){
		///delete the old SceneGraphNode and add this one instead
		SAFE_UPDATE((result.first)->second,this);
	}
	//That's it. Parent Transforms will be updated in the next render pass;
}

///Add a new SceneGraphNode to the current node's child list based on a SceneNode
SceneGraphNode* SceneGraphNode::addNode(SceneNode* const node,const std::string& name){
	//Create a new SceneGraphNode with the SceneNode's info
	SceneGraphNode* sceneGraphNode = New SceneGraphNode(node);
	//Validate it to be safe
	assert(sceneGraphNode);
	//We need to name the new SceneGraphNode
	//We start off with the name passed as a parameter as it has a higher priority
	std::string sgName(name);
	//If we did not supply a custom name
	if(sgName.empty()){
		//Use the SceneNode's name
		sgName = node->getName();
	}
	//Name the new SceneGraphNode
	sceneGraphNode->setName(sgName);

	//Get our current's node parent
	SceneGraphNode* parentNode = getParent();
	//Set the current node's parent as the new node's grandparent
	sceneGraphNode->setGrandParent(parentNode);
	//Get the new node's transform
	Transform* nodeTransform = sceneGraphNode->getTransform();
	//If the current node and the new node have transforms,
	//Update the relationship between the 2
	if(nodeTransform && getTransform()){
		//The child node's parentMatrix is our current transform matrix
		nodeTransform->setParentMatrix(getTransform()->getMatrix());
	}

	//Set the current node as the new node's parrent
	sceneGraphNode->setParent(this);
    sceneGraphNode->setSceneGraph(_sceneGraph);
	//Do all the post load operations on the SceneNode
	//Pass a reference to the newly created SceneGraphNode in case we need transforms or bounding boxes
	node->postLoad(sceneGraphNode);
	//return the newly created node
	return sceneGraphNode;
}

//Remove a child node from this Node
void SceneGraphNode::removeNode(SceneGraphNode* node){
	//find the node in the children map
	NodeChildren::iterator it = _children.find(node->getName());
	//If we found the node we are looking for
	if(it != _children.end()) {
		//Remove it from the map
		_children.erase(it);
	}else{
		for_each(NodeChildren::value_type& childIt, _children){
			removeNode(node);
		}
	}
	//Beware. Removing a node, does no unload it!
	//Call delete on the SceneGraphNode's pointer to do that
}

//Finding a node based on the name of the SceneGraphNode or the SceneNode it holds
//Switching is done by setting sceneNodeName to false if we pass a SceneGraphNode name
//or to true if we search by a SceneNode's name
SceneGraphNode* SceneGraphNode::findNode(const std::string& name, bool sceneNodeName){
	//Null return value as default
	SceneGraphNode* returnValue = NULL;
	 //Make sure a name exists
	if (!name.empty()){
		//check if it is the name we are looking for
		//If we are searching for a SceneNode ...
		if(sceneNodeName){
			if (_node->getName().compare(name) == 0){
				// We got the node!
				return this;
			}
		}else{ //If we are searching for a SceneGraphNode ...
			if (getName().compare(name) == 0){
				// We got the node!
				return this;
			}
		}

		//The current node isn't the one we wan't, so recursively check all children
		for_each(NodeChildren::value_type& it, _children){
			returnValue = it.second->findNode(name);
				if(returnValue != NULL){
					// if it is not NULL it is the node we are looking for
					// so just pass it through
					return returnValue;
			}
		}
	}

    // no children's name matches or there are no more children
    // so return NULL, indicating that the node was not found yet
    return NULL;
}

SceneGraphNode* SceneGraphNode::Intersect(const Ray& ray, F32 start, F32 end){
	//Null return value as default
	SceneGraphNode* returnValue = NULL;
    ReadLock r_lock(_queryLock);
	if(_boundingBox.Intersect(ray,start,end)){
		return this;
	}
    r_lock.unlock();

	for_each(NodeChildren::value_type& it, _children){
		returnValue = it.second->Intersect(ray,start,end);
		if(returnValue != NULL){
			// if it is not NULL it is the node we are looking for
			// so just pass it through
			return returnValue;
		}
	}

	return returnValue;
}

//This updates the SceneGraphNode's transform by deleting the old one first
void SceneGraphNode::setTransform(Transform* const t) {
	SAFE_UPDATE(_transform,t);
}

//Get the node's transform
Transform* const SceneGraphNode::getTransform(){
	//A node does not necessarily have a transform
	//If this is the case, we can either create a default one
	//Or return NULL. When creating a node we can specify if we do not want a default transform
	if(!_noDefaultTransform && !_transform){
		_transform = New Transform();
		assert(_transform);
        _isReady = true;
	}
	return _transform;
}

void SceneGraphNode::setNavigationContext(const NavigationContext& newContext) {
	_navigationContext = newContext;
	for_each(NodeChildren::value_type& it, _children){
		it.second->setNavigationContext(_navigationContext);
	}
}

void  SceneGraphNode::setNavigationDetailOverride(const bool detailOverride){
	_overrideNavMeshDetail = detailOverride;
	for_each(NodeChildren::value_type& it, _children){
		it.second->setNavigationDetailOverride(detailOverride);
	}
}