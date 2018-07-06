#include "Headers/SkinnedMesh.h"
#include "Headers/SkinnedSubMesh.h"
#include "Core/Headers/ParamHandler.h"

void SkinnedMesh::sceneUpdate(U32 sceneTime){
	///sceneTime is in milliseconds. Convert to seconds
	D32 timeIndex = getMsToSec(sceneTime);
	for_each(subMeshRefMap::value_type& subMesh, _subMeshRefMap){
		dynamic_cast<SkinnedSubMesh*>(subMesh.second)->updateAnimations(timeIndex);
	}
}

bool SkinnedMesh::playAnimations() {
	if(_playAnimations && ParamHandler::getInstance().getParam<bool>("mesh.playAnimations")){
		return true;
	}

	return false;
}
