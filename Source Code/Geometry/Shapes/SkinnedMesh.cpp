#include "Headers/SkinnedMesh.h"
#include "Headers/SkinnedSubMesh.h"

#include "Core/Headers/ParamHandler.h"
#include "Graphs/Headers/SceneGraphNode.h"

void SkinnedMesh::sceneUpdate(const D32 deltaTime,SceneGraphNode* const sgn, SceneState& sceneState){
    ///sceneTime is in milliseconds. Convert to seconds
    for_each(subMeshRefMap::value_type& subMesh, _subMeshRefMap){
        assert(subMesh.second);
        dynamic_cast<SkinnedSubMesh*>(subMesh.second)->updateAnimations(deltaTime,sgn);
    }
    Object3D::sceneUpdate(deltaTime,sgn,sceneState);
}

bool SkinnedMesh::playAnimations() {
    if(_playAnimations && ParamHandler::getInstance().getParam<bool>("mesh.playAnimations")){
        return true;
    }

    return false;
}

void SkinnedMesh::updateTransform(SceneGraphNode* const sgn){
}