#include "SubMesh.h"
#include "Managers/ResourceManager.h"

bool SubMesh::computeBoundingBox(){
	_boundingBox.setMin(vec3(100000.0f, 100000.0f, 100000.0f));
	_boundingBox.setMax(vec3(-100000.0f, -100000.0f, -100000.0f));

	std::vector<vec3>&	tPosition	= _geometry->getPosition();

	for(U32 i=0; i < tPosition.size(); i++){
		_boundingBox.Add( tPosition[i] );
	}
	setOriginalBoundingBox(_boundingBox);
	_boundingBox.isComputed() = true;
	return true;
}


bool SubMesh::unload(){
	getIndices().clear(); 
	return SceneNode::unload();
}