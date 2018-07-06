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
}

Mesh::~Mesh()
{
    MemoryManager::DELETE(_animator);
}

/// Mesh bounding box is built from all the SubMesh bounding boxes
bool Mesh::computeBoundingBox(SceneGraphNode& sgn) {
    BoundingBox& bb = sgn.getBoundingBox();

    bb.reset();
    for (SceneGraphNode_ptr child : sgn.getChildren()) {
        if (isSubMesh(child)) {
            bb.Add(child->getInitialBoundingBox());
        } else {
            bb.Add(child->getBoundingBoxConst());
        }
    }
    return SceneNode::computeBoundingBox(sgn);
}

bool Mesh::isSubMesh(const SceneGraphNode_ptr& node) {
    I64 targetGUID = node->getNode()->getGUID();
    return std::find_if(std::begin(_subMeshList),
                        std::end(_subMeshList),
                        [&targetGUID](SubMesh* const submesh) {
                            return (submesh && submesh->getGUID() == targetGUID);
                        }) != std::cend(_subMeshList);
}

void Mesh::addSubMesh(SubMesh* const subMesh) {
    // Hold a reference to the submesh by ID (used for animations)
    _subMeshList.push_back(subMesh);
    Attorney::SubMeshMesh::setParentMesh(*subMesh, this);
    _maxBoundingBox.reset();
}

/// After we loaded our mesh, we need to add submeshes as children nodes
void Mesh::postLoad(SceneGraphNode& sgn) {
    for (SubMesh* submesh : _subMeshList) {
        sgn.addNode(*submesh,
                    Util::StringFormat("%s_%d",
                                        sgn.getName().c_str(),
                                        submesh->getID()));
    }

    Object3D::postLoad(sgn);
}

/// Called from SceneGraph "sceneUpdate"
void Mesh::sceneUpdate(const U64 deltaTime, SceneGraphNode& sgn,
                       SceneState& sceneState) {
    if (hasFlag(ObjectFlag::OBJECT_FLAG_SKINNED)) {
        bool playAnimations = sceneState.renderState().playAnimations() && _playAnimations;
        for (SceneGraphNode_ptr child : sgn.getChildren()) {
            AnimationComponent* comp = child->getComponent<AnimationComponent>();
            // Not all submeshes are necessarily animated. (e.g. flag on the back of a character)
            if (comp) {
                comp->playAnimations(playAnimations && comp->playAnimations());
                comp->incParentTimeStamp(deltaTime);
            }
        }
    }

    SceneNode::sceneUpdate(deltaTime, sgn, sceneState);
}
};