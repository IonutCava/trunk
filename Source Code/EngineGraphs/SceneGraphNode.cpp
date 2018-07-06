#include "SceneGraphNode.h"
#include "Utility/Headers/BaseClasses.h"
#include "Terrain/Terrain.h"
#include "Terrain/Water.h"
#include "Hardware/Video/Light.h"
#include "Geometry/Object3D.h"
#include "Managers/SceneManager.h"

SceneGraphNode::SceneGraphNode(SceneNode* node) : _node(node), 
												  _parent(NULL),
												  _grandParent(NULL),
												  _wasActive(true),
											      _active(true),
								                  _transform(NULL),
								                  _noDefaultTransform(false),
												  _inView(true),
												  _sorted(false),
												  _sceneGraph(NULL),
												  _updateTimer(GETMSTIME()),
												  _childQueue(0)
{
}


SceneGraphNode::~SceneGraphNode(){
	if(_transform != NULL) {
		delete _transform;
		_transform = NULL;
	} 
}

bool SceneGraphNode::unload(){
	foreach(NodeChildren::value_type it, _children){
		if(it.second->unload()){

		}else{
			Console::getInstance().errorfn("SceneGraphNode: Could not unload node [ %s ]",it.second->getNode()->getName().c_str());
			return false;
		}
	}
	//Console::getInstance().printfn("Removing: %s (%s)",_node->getName().c_str(), getName().c_str());
	_children.clear();

	if(getParent()){ //if not root
		RemoveResource(_node);
	}
	return true;
}

void SceneGraphNode::print(){
	SceneGraphNode* parent = this;
	U8 i = 0;
	while(parent != NULL){
		parent = parent->getParent();
		i++;
	}
	Material* mat = getNode()->getMaterial();
	std::string material("none"),shader("none");
	if(mat){
		material = mat->getName();
		if(mat->getShader()){
			shader = mat->getShader()->getName();
		}
	}
	Console::getInstance().printfn("%s (Resource: %s, Material: %s (Shader: %s) )", getName().c_str(),getNode()->getName().c_str(),material.c_str(),shader.c_str());
	foreach(NodeChildren::value_type it, _children){
		for(U8 j = 0; j < i; j++){
			Console::getInstance().printf("-");
		}
		it.second->print();
	}
		
}

SceneGraphNode* SceneGraphNode::addNode(SceneNode* node,const std::string& name){
	SceneGraphNode* sceneGraphNode = New SceneGraphNode(node);
	SceneGraphNode* parentNode = this->getParent();
	assert(sceneGraphNode);
	sceneGraphNode->setParent(this);
	sceneGraphNode->setGrandParent(parentNode);

	Transform* nodeTransform = sceneGraphNode->getTransform();

	if(parentNode ){
		if(parentNode->getParent()){//If we have a parent, and our parent isn't root ..
			Transform* parentTransform = parentNode->getTransform(); // ... get this parent's transform ...
			if(nodeTransform && parentTransform){ // ... and as long as the parent has a transform ...
				nodeTransform->setParentMatrix(parentTransform->getMatrix()); // ... store it as a parent matrix ...
			}
		}
	}

	std::pair<unordered_map<std::string, SceneGraphNode*>::iterator, bool > result;
	std::string sgName(name);
	
	if(sgName.empty()){
		sgName = node->getName();
	}
	result = _children.insert(std::make_pair(sgName,sceneGraphNode));
	if(!result.second){
		delete (result.first)->second; 
		(result.first)->second = sceneGraphNode;
	}
	sceneGraphNode->setName(sgName);
	node->postLoad(sceneGraphNode);
	return sceneGraphNode;
}

SceneGraphNode* SceneGraphNode::findNode(const std::string& name){
	if(_node->getName().compare(name) == 0){ //if the current node is what we are looking for, return it
		return this;
	}else{ //else, check the immediat children if they match
		NodeChildren::iterator findResult = _children.find(name);
		if(findResult != _children.end()){
			return findResult->second;
		}else{ //else, for each child, check it's children
			foreach(NodeChildren::value_type it, _children){
				SceneGraphNode* result = it.second->findNode(name);
				if(result) return result;

			}
		}
	}
	return NULL; //finally ... return nothing
}

void SceneGraphNode::updateTransformsAndBounds(){
	//if(GETMSTIME() - _updateTimer  < 10) return; //update every ten milliseconds
	//Update from bottom up, so that child BB's are added to parrent BoundingBox;
	foreach(NodeChildren::value_type &it, _children){
		it.second->updateTransformsAndBounds();
	}
	//update/compute the bounding box
	if(!_boundingBox.isComputed()){
		_node->computeBoundingBox(this);
	}
	//transform the bounding box
	if(!_transform) return;	
	_boundingBox.Transform(_initialBoundingBox,_transform->getMatrix());			
	//for each of this node's children
	foreach(NodeChildren::value_type &it, _children){
		//set they'r parent matrix;
		Transform* t = it.second->getTransform();
		if(t){
			//update parent matrix (cached)
			t->setParentMatrix(_transform->getMatrix());
			//add current bounding box to parent
			//_boundingBox.Add(it.second->getBoundingBox());

		}
	}
	
	

	//_updateTimer = GETMSTIME();
}

void SceneGraphNode::updateVisualInformation(){
	Scene* curentScene = SceneManager::getInstance().getActiveScene();
	//Update from bottom up, so that child BB's are added to parrent BoundingBox;
	foreach(NodeChildren::value_type &it, _children){
		it.second->updateVisualInformation();
	}
	//if node isn't active, skip computations
	if(!_active) {
		_inView = false;
		return;
	}
	//Hold a pointer to the scenegraph
	if(!_sceneGraph){
		_sceneGraph = SceneManager::getInstance().getActiveScene()->getSceneGraph();
	}

	//chek if node is in view
	if(_node->isInView(true,_boundingBox)){
		_inView = (curentScene->drawObjects() && _active && _node->getRenderState());
	}
}

void SceneGraphNode::render(){
	Scene* curentScene = SceneManager::getInstance().getActiveScene();
	// If node isn't visible, return
	if(!_inView) return; 
	//prerender the node (compute shaders for example
	_node->onDraw();
	
	//draw bounding box if needed
	if(!GFXDevice::getInstance().getDepthMapRendering()){
		if(_boundingBox.getVisibility() || curentScene->drawBBox()){
			GFXDevice::getInstance().drawBox3D(_boundingBox.getMin(),_boundingBox.getMax());
		}
	}

	//setup materials and rener the node
	_node->prepareMaterial();
	_node->render(this); //render the current node only if it is visible. Render the children, regardless
	_node->releaseMaterial();
	//render all children
	foreach(NodeChildren::value_type &it, _children){
		it.second->render();
	}
}

Transform* SceneGraphNode::getTransform(){
	if(!_transform && !_noDefaultTransform){
		_transform = New Transform();
		assert(_transform);
	}
	return _transform;
}

template<class T>
T* SceneGraphNode::getNode(){return dynamic_cast<T>(_node);}
template<>
WaterPlane* SceneGraphNode::getNode<WaterPlane>(){ return dynamic_cast<WaterPlane*>(_node); }
template<>
Terrain* SceneGraphNode::getNode<Terrain>(){ return dynamic_cast<Terrain*>(_node); }
template<>
Light* SceneGraphNode::getNode<Light>(){ return dynamic_cast<Light*>(_node); }
template<>
Object3D* SceneGraphNode::getNode<Object3D>(){ return dynamic_cast<Object3D*>(_node); }
