#include "SceneGraphNode.h"
#include "Utility/Headers/BaseClasses.h"
#include "Terrain/Terrain.h"
#include "Terrain/Water.h"
#include "Hardware/Video/Light.h"
#include "Geometry/Object3D.h"

SceneGraphNode::~SceneGraphNode(){

}

bool SceneGraphNode::unload(){
	for(NodeChildren::iterator it = _children.begin(); it != _children.end(); it++){
		if(it->second->unload()){
			delete it->second;
			it->second = NULL;
		}else{
			Console::getInstance().errorfn("SceneGraphNode: Could not unload node [ %s ]",it->second->getNode()->getName().c_str());
			return false;
		}
	}
	_children.clear();
	if(getParent()) //if not root
		ResourceManager::getInstance().remove(_node);
	return true;
}

SceneGraphNode* SceneGraphNode::addNode(SceneNode* node){
	SceneGraphNode* sceneGraphNode = new SceneGraphNode(node);
	assert(sceneGraphNode);
	sceneGraphNode->setParent(this);
	sceneGraphNode->setGrandParent(this->getParent());

	std::pair<std::tr1::unordered_map<std::string, SceneGraphNode*>::iterator, bool > result;
	result = _children.insert(std::make_pair(node->getName(),sceneGraphNode));
	if(!result.second){
		delete (result.first)->second; 
		(result.first)->second = sceneGraphNode;
	}
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
			for(NodeChildren::iterator it = _children.begin(); it != _children.end(); it++){
				SceneGraphNode* result = it->second->findNode(name);
				if(result) return result;
			}
		}
	}
	return NULL; //finally ... return nothing
}

void SceneGraphNode::render(){
	if(!_active) return; //If the node is active, nothing beyond it  is rendered;
	if(_node->getRenderState()) _node->render(); //render the current node only if it is visible. Render the children, regardless
	//ToDo:: apply parrent transforms here! - Ionut;
	for(NodeChildren::iterator it = _children.begin(); it != _children.end(); it++){
		it->second->render();
	}
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
