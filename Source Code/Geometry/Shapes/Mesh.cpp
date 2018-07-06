#include "Headers/Mesh.h"
#include "Headers/SubMesh.h"

#include "Managers/Headers/SceneManager.h"
#include "Core/Math/Headers/Quaternion.h"
#include "Geometry/Animations/Headers/AnimationController.h"

using namespace std;

Mesh::~Mesh(){
	SAFE_DELETE(_animator);
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


/// Mesh bounding box is built from all the SubMesh bounding boxes
bool Mesh::computeBoundingBox(SceneGraphNode* const sgn){
	BoundingBox& bb = sgn->getBoundingBox();
	if(bb.isComputed()) return true;
	//bb.set(vec3<F32>(100000.0f, 100000.0f, 100000.0f), vec3<F32>(-100000.0f, -100000.0f, -100000.0f));
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
		subMeshSGN->getTransform()->setTransforms(sgn->getTransform()->getMatrix());
		///Hold a reference to the submesh by ID (used for animations)
		_subMeshRefMap.insert(std::make_pair(s->getId(), s));
		s->setParentMesh(this);
	}
	Object3D::postLoad(sgn);
	_loaded = true;
}

/// Create a mesh animator from assimp imported data
bool Mesh::createAnimatorFromScene(const aiScene* scene){
	assert(scene != NULL);
	/// Delete old animator if any
	SAFE_DELETE(_animator);
	_animator = New AnimationController();
	_animator->Init(scene);
	return true;
}

void Mesh::onDraw(){
	if(!_loaded) return;
	// update possible animation
	assert(_animator != NULL);

	if (_playAnimations && _animator->HasSkeleton() && _animator->HasAnimations()) {
		_transforms = _animator->GetTransforms(Application::getInstance().getCurrentTime());
	}
	Object3D::onDraw();
}