#include "Headers/SubMesh.h"
#include "Headers/Mesh.h"
#include "Managers/Headers/ResourceManager.h"


bool SubMesh::computeBoundingBox(SceneGraphNode* const sgn){
	BoundingBox& bb = sgn->getBoundingBox();
	if(bb.isComputed()) return true;
	bb.set(vec3<F32>(100000.0f, 100000.0f, 100000.0f),vec3<F32>(-100000.0f, -100000.0f, -100000.0f));

	std::vector<vec3<F32> >&	tPosition	= _geometry->getPosition();

	for(U32 i=0; i < tPosition.size(); i++){
		bb.Add( tPosition[i] );
	}

	return SceneNode::computeBoundingBox(sgn);
}


bool SubMesh::unload(){
	getIndices().clear(); 
	return SceneNode::unload();
}

void SubMesh::onDraw(){
	//std::vector<mat4<F32> >& transforms = _parentMesh->GetTransforms();
	//if(!transforms.empty()){
	//	std::vector<vec3<F32> >& verts   = _geometry->getPosition();
	//	//std::vector<vec3<F32> >& normals = _geometry->getNormal();
	//	std::vector<vec4<I16> >& indices = _geometry->getBoneIndices();
	//	std::vector<vec4<F32> >& weights = _geometry->getBoneWeights();

	//	/// loop through all vertex weights of all bones 
	//	for( size_t i = 0; i < verts.size(); ++i) { 
	//		vec3<F32>& pos = verts[i];
	//		//vec3<F32>& nor = normals[i];

	//		vec4<I16> ind = indices[i];
	//		vec4<F32> wgh = weights[i];


	//		vec4<F32> pos1 = wgh.x * (transforms[ind.x] * pos);
	//		vec4<F32> pos2 = wgh.y * (transforms[ind.y] * pos);
	//		vec4<F32> pos3 = wgh.z * (transforms[ind.z] * pos);
	//		vec4<F32> pos4 = wgh.w * (transforms[ind.w] * pos);
	//		vec4<F32> finalPosition =  pos1 + pos2 + pos3 + pos4;
	//		verts[i] = finalPosition;

	//	}
	//	_geometry->Refresh();
	//}
	Object3D::onDraw();
}