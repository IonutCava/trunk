#include "Headers/SkinnedMesh.h"
#include "Headers/SkinnedSubMesh.h"

#include "Core/Headers/ParamHandler.h"
#include "Graphs/Headers/SceneGraphNode.h"

SkinnedMesh::SkinnedMesh() : Mesh(OBJECT_FLAG_SKINNED), _playAnimation(true)
{

}

SkinnedMesh::~SkinnedMesh()
{

}

void SkinnedMesh::sceneUpdate(const U64 deltaTime, SceneGraphNode* const sgn, SceneState& sceneState){
    bool playAnimation = (_playAnimation && ParamHandler::getInstance().getParam<bool>("mesh.playAnimations"));

    FOR_EACH(subMeshRefMap::value_type& subMesh, _subMeshRefMap){
        dynamic_cast<SkinnedSubMesh*>(subMesh.second)->playAnimation(playAnimation);
    }

    Object3D::sceneUpdate(deltaTime,sgn,sceneState);
}

/// Select next available animation
bool SkinnedMesh::playNextAnimation(){
    bool success = true;
    FOR_EACH(subMeshRefMap::value_type& subMesh, _subMeshRefMap){
        if(!dynamic_cast<SkinnedSubMesh*>(subMesh.second)->playNextAnimation())
            success =  false;
    }
    return success;
}

/// Select an animation by index
bool SkinnedMesh::playAnimation(I32 index){
    bool success = true;
    FOR_EACH(subMeshRefMap::value_type& subMesh, _subMeshRefMap){
        if(!dynamic_cast<SkinnedSubMesh*>(subMesh.second)->playAnimation(index))
            success =  false;
    }

    return success;
}

/// Select an animation by name
bool SkinnedMesh::playAnimation(const std::string& animationName){
    bool success = true;
    FOR_EACH(subMeshRefMap::value_type& subMesh, _subMeshRefMap){
        if(!dynamic_cast<SkinnedSubMesh*>(subMesh.second)->playAnimation(animationName))
            success =  false;
    }
    return success;
}