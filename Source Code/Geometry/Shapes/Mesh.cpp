#include "Headers/Mesh.h"
#include "Headers/SubMesh.h"
#include "Managers/Headers/SceneManager.h"

using namespace std;

Mesh::~Mesh(){
}

bool Mesh::load(const string& name){
	_name = name;
	return true;
}

void Mesh::removeCopy(){
	SceneNode::removeCopy();
}

void Mesh::createCopy(){
	SceneNode::createCopy();
	for_each(std::string& it, _subMeshes){
		Resource* s = FindResource(it);
		if(s) s->createCopy();
	}
}

void Mesh::updateBBatCurrentFrame(SceneGraphNode* const sgn){
	if(_updateBB){
		BoundingBox& bb = sgn->getBoundingBox();
		bb.set(vec3<F32>(100000.0f, 100000.0f, 100000.0f),vec3<F32>(-100000.0f, -100000.0f, -100000.0f));
		for_each(childrenNodes::value_type& s, sgn->getChildren()){
			bb.Add(s.second->getBoundingBox());
		}
		SceneNode::computeBoundingBox(sgn);
		_updateBB = false;
	}
}

/// Mesh bounding box is built from all the SubMesh bounding boxes
bool Mesh::computeBoundingBox(SceneGraphNode* const sgn){
	BoundingBox& bb = sgn->getBoundingBox();
	if(bb.isComputed()) return true;
	return SceneNode::computeBoundingBox(sgn);
}

/// After we loaded our mesh, we need to add submeshes as children nodes
void Mesh::postLoad(SceneGraphNode* const sgn){
	for_each(std::string& it, _subMeshes){
		ResourceDescriptor subMesh(it);
		/// Find the SubMesh resource
		SubMesh* s = dynamic_cast<SubMesh*>(FindResource(it));
		if(!s) continue;
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
	_loaded = true;
}

void Mesh::onDraw(){
	if(!_loaded) return;

	Object3D::onDraw();
}


void Mesh::updateTransform(SceneGraphNode* const sgn){
}

/// Called from SceneGraph "sceneUpdate"
void Mesh::sceneUpdate(D32 sceneTime){
}

