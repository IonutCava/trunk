#include "Headers/Mesh.h"

#include "Managers/Headers/SceneManager.h"
#include "Core/Math/Headers/Quaternion.h"

using namespace std;

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


/// Mesh bounding box is built from all the SubMesh bounding boxes
bool Mesh::computeBoundingBox(SceneGraphNode* const sgn){
	BoundingBox& bb = sgn->getBoundingBox();
	if(bb.isComputed()) return true;
	//bb.set(vec3(100000.0f, 100000.0f, 100000.0f), vec3(-100000.0f, -100000.0f, -100000.0f));
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
		PRINT_FN("Adding [ %s ] to [ %s ]" , string(sgn->getName()+"_"+it).c_str(),sgn->getName().c_str());
		SceneGraphNode* subMeshSGN  = sgn->addNode(s,sgn->getName()+"_"+it);
		//Set SubMesh transform to this transform - HACK. ToDo <- Fix this. Use parent matrix
		subMeshSGN->getTransform()->setTransforms(sgn->getTransform()->getMatrix());
	}
	Object3D::postLoad(sgn);
}