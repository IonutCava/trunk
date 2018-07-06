#include "Headers/SubMesh.h"
#include "Headers/Mesh.h"
#include "Core/Resources/Headers/ResourceCache.h"
#include "Core/Headers/ParamHandler.h"

SubMesh::~SubMesh(){
}

bool SubMesh::computeBoundingBox(SceneGraphNode* const sgn){
	BoundingBox& bb = sgn->getBoundingBox();
	if(bb.isComputed()) return true;
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

void SubMesh::onDraw(){
	Object3D::onDraw();
}

/// Called from SceneGraph "sceneUpdate"
void SubMesh::sceneUpdate(U32 sceneTime){
}
void SubMesh::updateTransform(SceneGraphNode* const sgn){
}
void SubMesh::updateBBatCurrentFrame(SceneGraphNode* const sgn){
}




