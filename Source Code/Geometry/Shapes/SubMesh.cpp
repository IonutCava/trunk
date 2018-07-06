#include "Headers/SubMesh.h"
#include "Headers/Mesh.h"

#include "Core/Resources/Headers/ResourceCache.h"
#include "Core/Math/Headers/Transform.h"
#include "Core/Headers/ParamHandler.h"
#include "Graphs/Headers/SceneGraphNode.h"

SubMesh::~SubMesh(){
}

bool SubMesh::computeBoundingBox(SceneGraphNode* const sgn){
	BoundingBox& bb = sgn->getBoundingBox();
	if(bb.isComputed())
		return true;

	bb.set(getGeometryVBO()->getMinPosition(),getGeometryVBO()->getMaxPosition());
	return SceneNode::computeBoundingBox(sgn);
}

/// After we loaded our mesh, we need to add submeshes as children nodes
void SubMesh::postLoad(SceneGraphNode* const sgn){
	//sgn->getTransform()->setTransforms(_localMatrix);
	/// If the mesh has animation data, use dynamic VBO's if we use software skinning
	getGeometryVBO()->Create();
	Object3D::postLoad(sgn);
}

void SubMesh::onDraw(const RenderStage& currentStage){
	Object3D::onDraw(currentStage);
}

/// Called from SceneGraph "sceneUpdate"
void SubMesh::sceneUpdate(const D32 deltaTime, SceneGraphNode* const sgn, SceneState& sceneState){
	Object3D::sceneUpdate(deltaTime, sgn,sceneState);
}

void SubMesh::updateTransform(SceneGraphNode* const sgn){
	Transform* parentTransform = sgn->getParent()->getTransform();
	//a mesh should always have a transform
	assert(parentTransform);
	if(parentTransform->isDirty()){
		sgn->updateBoundingBoxTransform(parentTransform->getGlobalMatrix());
	}
}

void SubMesh::updateBBatCurrentFrame(SceneGraphNode* const sgn){
}