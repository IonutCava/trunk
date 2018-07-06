#include "Headers/SubMesh.h"
#include "Headers/Mesh.h"

#include "Core/Resources/Headers/ResourceCache.h"
#include "Core/Math/Headers/Transform.h"
#include "Core/Headers/ParamHandler.h"
#include "Graphs/Headers/SceneGraphNode.h"

// possibly keep per submesh vbs and use them only for data manipulation and not for rendering
#pragma message("TODO (Prio 1): - Use only 1 VB per mesh and use per submesh offset to said VB")
#pragma message("               - Use texture atlas to store all textures in one (or a few) files in the Mesh class")
#pragma message("               - Ionut")

SubMesh::~SubMesh(){
}

bool SubMesh::computeBoundingBox(SceneGraphNode* const sgn){
	BoundingBox& bb = sgn->getBoundingBox();
	if(bb.isComputed())
		return true;

	bb.set(getGeometryVB()->getMinPosition(),getGeometryVB()->getMaxPosition());
	return SceneNode::computeBoundingBox(sgn);
}

/// After we loaded our mesh, we need to add submeshes as children nodes
void SubMesh::postLoad(SceneGraphNode* const sgn){
	//sgn->getTransform()->setTransforms(_localMatrix);
	/// If the mesh has animation data, use dynamic VB's if we use software skinning
	getGeometryVB()->Create();
	Object3D::postLoad(sgn);
}
