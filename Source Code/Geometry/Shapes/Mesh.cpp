#include "Headers/Mesh.h"
#include "Headers/SubMesh.h"
#include "Managers/Headers/SceneManager.h"

void Mesh::updateBBatCurrentFrame(SceneGraphNode* const sgn){
	if(!ParamHandler::getInstance().getParam<bool>("mesh.playAnimations")) return;
	if(sgn->updateBB()){
		BoundingBox& bb = sgn->getBoundingBox();
		bb.set(vec3<F32>(100000.0f, 100000.0f, 100000.0f),vec3<F32>(-100000.0f, -100000.0f, -100000.0f));
		for_each(childrenNodes::value_type& s, sgn->getChildren()){
			bb.Add(s.second->getBoundingBox());
		}
		SceneNode::computeBoundingBox(sgn);
		SceneNode::updateBBatCurrentFrame(sgn);
	}
}

/// Mesh bounding box is built from all the SubMesh bounding boxes
bool Mesh::computeBoundingBox(SceneGraphNode* const sgn){
	BoundingBox& bb = sgn->getBoundingBox();
	if(bb.isComputed()) return true;
	bb.set(vec3<F32>(100000.0f, 100000.0f, 100000.0f),vec3<F32>(-100000.0f, -100000.0f, -100000.0f));
	for_each(childrenNodes::value_type& s, sgn->getChildren()){
		bb.Add(s.second->getBoundingBox());
	}
	return SceneNode::computeBoundingBox(sgn);
}

/// After we loaded our mesh, we need to add submeshes as children nodes
void Mesh::postLoad(SceneGraphNode* const sgn){
	for_each(std::string& it, _subMeshes){
		ResourceDescriptor subMesh(it);
		/// Find the SubMesh resource
		SubMesh* s = dynamic_cast<SubMesh*>(FindResource(it));
		if(!s) continue;
		REGISTER_TRACKED_DEPENDENCY(s);

		/// Add the SubMesh resource as a child
		SceneGraphNode* subMeshSGN  = sgn->addNode(s,sgn->getName()+"_"+it);
		///Set SubMesh transform to this transform - HACK. ToDo <- Fix this. Use parent matrix
		subMeshSGN->getTransform()->setParentMatrix(sgn->getTransform()->getMatrix());
		///Hold a reference to the submesh by ID (used for animations)
		_subMeshRefMap.insert(std::make_pair(s->getId(), s));
		s->setParentMesh(this);
		if(s->_hasAnimations){
			_hasAnimations = true;
		}
	}
	Object3D::postLoad(sgn);
}

void Mesh::onDraw(){

	if(!isLoaded()) return;
	_playAnimations = ParamHandler::getInstance().getParam<bool>("mesh.playAnimations");
	Object3D::onDraw();
}


void Mesh::updateTransform(SceneGraphNode* const sgn){
}

/// Called from SceneGraph "sceneUpdate"
void Mesh::sceneUpdate(D32 sceneTime){
	///sceneTime is in miliseconds. Convert to seconds
	D32 timeIndex = sceneTime/1000.0;
	for_each(subMeshRefMap::value_type& subMesh, _subMeshRefMap){
		subMesh.second->updateAnimations(timeIndex);
	}
}

