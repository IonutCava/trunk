#include "Headers/Mesh.h"
#include "Headers/SubMesh.h"

#include "Managers/Headers/SceneManager.h"
#include "Core/Headers/StringHelper.h"
#include "Core/Math/Headers/Transform.h"
#include "Geometry/Animations/Headers/SceneAnimator.h"

namespace Divide {

Mesh::Mesh(GFXDevice& context,
           ResourceCache& parentCache,
           size_t descriptorHash,
           const stringImpl& name,
           const stringImpl& resourceName,
           const stringImpl& resourceLocation, 
           ObjectFlag flag)
    : Object3D(context, parentCache, descriptorHash, name, resourceName, resourceLocation, ObjectType::MESH, flag),
      _visibleToNetwork(true),
      _animator(nullptr)
{
}

Mesh::~Mesh()
{
}

void Mesh::addSubMesh(SubMesh_ptr subMesh) {
    // Hold a reference to the submesh by ID (used for animations)
    _subMeshList.push_back(subMesh);
    Attorney::SubMeshMesh::setParentMesh(*subMesh.get(), this);
    setFlag(UpdateFlag::BOUNDS_CHANGED);
}

void Mesh::updateBoundsInternal(SceneGraphNode& sgn) {
    _boundingBox.reset();
}

/// After we loaded our mesh, we need to add submeshes as children nodes
void Mesh::postLoad(SceneGraphNode& sgn) {
    static const U32 normalMask = to_const_U32(SGNComponent::ComponentType::NAVIGATION) |
                                  to_const_U32(SGNComponent::ComponentType::PHYSICS) |
                                  to_const_U32(SGNComponent::ComponentType::BOUNDS) |
                                  to_const_U32(SGNComponent::ComponentType::RENDERING) |
                                  to_const_U32(SGNComponent::ComponentType::NAVIGATION);

    static const U32 skinnedMask = normalMask | 
                                   to_const_U32(SGNComponent::ComponentType::ANIMATION) |
                                   to_const_U32(SGNComponent::ComponentType::INVERSE_KINEMATICS) |
                                   to_const_U32(SGNComponent::ComponentType::RAGDOLL);

    for (const SubMesh_ptr& submesh : _subMeshList) {
        sgn.addNode(submesh,
                    submesh->getObjectFlag(ObjectFlag::OBJECT_FLAG_SKINNED) ? skinnedMask : normalMask,
                    PhysicsGroup::GROUP_IGNORE,
                    Util::StringFormat("%s_%d",
                                        sgn.getName().c_str(),
                                        submesh->getID()));
    }
    sgn.get<BoundsComponent>()->lockBBTransforms(true);
    Object3D::postLoad(sgn);
}

/// Called from SceneGraph "sceneUpdate"
void Mesh::sceneUpdate(const U64 deltaTime, SceneGraphNode& sgn,
                       SceneState& sceneState) {
    if (getObjectFlag(ObjectFlag::OBJECT_FLAG_SKINNED)) {
        bool playAnimations = sceneState.renderState().isEnabledOption(SceneRenderState::RenderOptions::PLAY_ANIMATIONS) &&
                              _playAnimations;
        sgn.forEachChild([playAnimations, deltaTime](const SceneGraphNode& child) {
            AnimationComponent* comp = child.get<AnimationComponent>();
            // Not all submeshes are necessarily animated. (e.g. flag on the back of a character)
            if (comp) {
                comp->playAnimations(playAnimations && comp->playAnimations());
                comp->incParentTimeStamp(deltaTime);
            }
        });
    }

    Object3D::sceneUpdate(deltaTime, sgn, sceneState);
}

};