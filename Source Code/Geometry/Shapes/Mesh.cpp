#include "Headers/Mesh.h"
#include "Headers/SubMesh.h"

#include "Managers/Headers/SceneManager.h"
#include "Core/Headers/ParamHandler.h"
#include "Core/Math/Headers/Transform.h"

namespace Divide {

Mesh::Mesh(ObjectFlag flag) : Object3D(MESH,flag),
                              _visibleToNetwork(true),
                              _playAnimations(true),
                              _playAnimationsCurrent(false)
{
    setState(RES_LOADING);
}

Mesh::~Mesh()
{
}

/// Mesh bounding box is built from all the SubMesh bounding boxes
bool Mesh::computeBoundingBox(SceneGraphNode* const sgn){
    BoundingBox& bb = sgn->getBoundingBox();

    bb.reset();
    for (SceneGraphNode::NodeChildren::value_type s : sgn->getChildren() ) {
        bb.Add( s.second->getInitialBoundingBox() );
    }
    bb.setComputed(true);

    return SceneNode::computeBoundingBox(sgn);
}

void Mesh::addSubMesh(SubMesh* const subMesh){
    //Hold a reference to the submesh by ID (used for animations)
    hashAlg::emplace(_subMeshRefMap, subMesh->getId(), subMesh);
    subMesh->setParentMesh(this);
    _maxBoundingBox.reset();
}

/// After we loaded our mesh, we need to add submeshes as children nodes
void Mesh::postLoad(SceneGraphNode* const sgn){
    for (SubMeshRefMap::value_type it : _subMeshRefMap ) {
        sgn->addNode(it.second, sgn->getName() + "_" + stringAlg::toBase(Util::toString(it.first)));
    }

    Object3D::postLoad(sgn);
}

/// Called from SceneGraph "sceneUpdate"
void Mesh::sceneUpdate(const U64 deltaTime, SceneGraphNode* const sgn, SceneState& sceneState){

    if ( bitCompare( getFlagMask(), OBJECT_FLAG_SKINNED ) ) {
        bool playAnimation = ( _playAnimations && ParamHandler::getInstance().getParam<bool>( "mesh.playAnimations" ) );
        if ( playAnimation != _playAnimationsCurrent ) {
            for ( SceneGraphNode::NodeChildren::value_type it : sgn->getChildren() ) {
                it.second->getComponent<AnimationComponent>()->playAnimation( playAnimation );
            }
            _playAnimationsCurrent = playAnimation;
        }
    }

    SceneNode::sceneUpdate(deltaTime, sgn, sceneState);
}

};