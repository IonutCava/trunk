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

    FOR_EACH(SceneGraphNode::NodeChildren::value_type& it, sgn->getChildren()){
        it.second->updateAnimations(playAnimation);
    }
    Object3D::sceneUpdate(deltaTime, sgn, sceneState);
}