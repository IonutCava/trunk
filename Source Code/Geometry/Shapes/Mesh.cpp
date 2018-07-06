#include "Headers/Mesh.h"
#include "Headers/SubMesh.h"

#include "Managers/Headers/SceneManager.h"
#include "Core/Headers/ParamHandler.h"
#include "Core/Math/Headers/Transform.h"
#include "Geometry/Animations/Headers/SceneAnimator.h"

namespace Divide {

Mesh::Mesh(ObjectFlag flag)
    : Object3D(ObjectType::MESH, flag),
      _visibleToNetwork(true),
      _animator(nullptr)
{
    setState(ResourceState::RES_LOADING);

    if (isSkinned()) {
        _animator = MemoryManager_NEW SceneAnimator();
    }
}

Mesh::~Mesh()
{
    MemoryManager::DELETE(_animator);
}

void Mesh::initAnimator(const aiScene* scene) {
    assert(isSkinned());

    _animator->init(scene);
}

/// Mesh bounding box is built from all the SubMesh bounding boxes
bool Mesh::computeBoundingBox(SceneGraphNode& sgn) {
    typedef SceneGraphNode::NodeChildren::value_type value_type;
    BoundingBox& bb = sgn.getBoundingBox();

    bb.reset();
    for (value_type it : sgn.getChildren()) {
        bb.Add(it.second->getInitialBoundingBox());
    }
    bb.setComputed(true);

    return SceneNode::computeBoundingBox(sgn);
}

void Mesh::addSubMesh(SubMesh* const subMesh) {
    // Hold a reference to the submesh by ID (used for animations)
    hashAlg::emplace(_subMeshRefMap, subMesh->getID(), subMesh);
    Attorney::SubMeshMesh::setParentMesh(*subMesh, this);
    _maxBoundingBox.reset();
}

/// After we loaded our mesh, we need to add submeshes as children nodes
void Mesh::postLoad(SceneGraphNode& sgn) {
    for (SubMeshRefMap::value_type it : _subMeshRefMap) {
        sgn.addNode(it.second, sgn.getName() + "_" + std::to_string(it.first));
    }

    Object3D::postLoad(sgn);
}

/// Called from SceneGraph "sceneUpdate"
void Mesh::sceneUpdate(const U64 deltaTime, SceneGraphNode& sgn,
                       SceneState& sceneState) {
    typedef SceneGraphNode::NodeChildren::value_type value_type;

    if (isSkinned()) {
        bool playAnimations =
            ParamHandler::getInstance().getParam<bool>("mesh.playAnimations") &&
            _playAnimations;

        for (value_type it : sgn.getChildren()) {
            AnimationComponent* comp = it.second->getComponent<AnimationComponent>();
            comp->playAnimations(playAnimations);
            comp->incParentTimeStamp(deltaTime);
        }
    }

    SceneNode::sceneUpdate(deltaTime, sgn, sceneState);
}
};