#include "Headers/SkinnedMesh.h"
#include "Headers/SkinnedSubMesh.h"

#include "Core/Headers/ParamHandler.h"
#include "Graphs/Headers/SceneGraphNode.h"

void SkinnedMesh::sceneUpdate(const U32 sceneTime,SceneGraphNode* const sgn, SceneState& sceneState){
    ///sceneTime is in milliseconds. Convert to seconds
    D32 timeIndex = getMsToSec(sceneTime);
    for_each(subMeshRefMap::value_type& subMesh, _subMeshRefMap){
        assert(subMesh.second);
        dynamic_cast<SkinnedSubMesh*>(subMesh.second)->updateAnimations(timeIndex,sgn);
    }
    Object3D::sceneUpdate(sceneTime,sgn,sceneState);
}

bool SkinnedMesh::playAnimations() {
    if(_playAnimations && ParamHandler::getInstance().getParam<bool>("mesh.playAnimations")){
        return true;
    }

    return false;
}

void SkinnedMesh::updateTransform(SceneGraphNode* const sgn){
}