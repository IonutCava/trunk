#include "stdafx.h"

#include "Headers/Mesh.h"
#include "Headers/SubMesh.h"

#include "Managers/Headers/SceneManager.h"
#include "Core/Headers/StringHelper.h"
#include "Geometry/Material/Headers/Material.h"
#include "Geometry/Animations/Headers/SceneAnimator.h"
#include "ECS/Components/Headers/RigidBodyComponent.h"
#include "ECS/Components/Headers/BoundsComponent.h"
#include "ECS/Components/Headers/RenderingComponent.h"
#include "ECS/Components/Headers/AnimationComponent.h"

namespace Divide {

Mesh::Mesh(GFXDevice& context,
           ResourceCache* parentCache,
           size_t descriptorHash,
           const Str128& name,
           const Str128& resourceName,
           const stringImpl& resourceLocation)
    : Object3D(context, parentCache, descriptorHash, name, resourceName, resourceLocation, ObjectType::MESH, 0),
      _visibleToNetwork(true),
      _animator(nullptr)
{
    _boundingBox.reset();
}

Mesh::~Mesh()
{
}

void Mesh::postImport() {
    recomputeBB();
}

void Mesh::recomputeBB() {
    if (_recomputeBBQueued) {
        _boundingBox.reset();
        for (const SubMesh_ptr& subMesh : _subMeshList) {
            _boundingBox.add(subMesh->getBounds());
        }

        setBounds(_boundingBox);
        _recomputeBBQueued = false;
    }
}

void Mesh::sceneUpdate(const U64 deltaTimeUS,
                       SceneGraphNode& sgn,
                       SceneState& sceneState) {

    recomputeBB();
    return Object3D::sceneUpdate(deltaTimeUS, sgn, sceneState);
}

void Mesh::addSubMesh(SubMesh_ptr subMesh) {
    // Hold a reference to the submesh by ID
    _subMeshList.push_back(subMesh);
    Attorney::SubMeshMesh::setParentMesh(*subMesh.get(), this);
}

void Mesh::setMaterialTpl(const Material_ptr& material) {
    Object3D::setMaterialTpl(material);

    for (const SubMesh_ptr& submesh : _subMeshList) {
        if (material != nullptr) {
            const Material_ptr& submeshMaterial = submesh->getMaterialTpl();
            if (submeshMaterial != nullptr) {
                submeshMaterial->baseShaderData(material->baseShaderData());
                for (U8 i = 0; i < to_base(ShaderType::COUNT); ++i) {
                    for (auto it : material->shaderDefines(static_cast<ShaderType>(i))) {
                        submeshMaterial->addShaderDefine(static_cast<ShaderType>(i), it.first, it.second);
                    }
                }
            }
        }
    }
}

/// After we loaded our mesh, we need to add submeshes as children nodes
void Mesh::postLoad(SceneGraphNode& sgn) {
    constexpr U32 normalMask = to_base(ComponentType::NAVIGATION) |
                               to_base(ComponentType::TRANSFORM) |
                               to_base(ComponentType::BOUNDS) |
                               to_base(ComponentType::RENDERING) |
                               to_base(ComponentType::NAVIGATION);

    constexpr U32 skinnedMask = normalMask |
                                to_base(ComponentType::ANIMATION) |
                                to_base(ComponentType::INVERSE_KINEMATICS) |
                                to_base(ComponentType::RAGDOLL);

    SceneGraphNodeDescriptor subMeshDescriptor;
    subMeshDescriptor._usageContext = sgn.usageContext();
    subMeshDescriptor._instanceCount = sgn.instanceCount();

    for (const SubMesh_ptr& submesh : _subMeshList) {
        const bool subMeshSkinned = submesh->getObjectFlag(ObjectFlag::OBJECT_FLAG_SKINNED);
        if (subMeshSkinned) {
            assert(getObjectFlag(ObjectFlag::OBJECT_FLAG_SKINNED));
        }

        subMeshDescriptor._node = submesh;
        subMeshDescriptor._componentMask = subMeshSkinned ? skinnedMask : normalMask;
        if (sgn.get<RigidBodyComponent>() != nullptr) {
            subMeshDescriptor._componentMask |= to_base(ComponentType::RIGID_BODY);
        }
        subMeshDescriptor._name = Util::StringFormat("%s_%d", sgn.name().c_str(), submesh->getID());
        SceneGraphNode* subSGN = sgn.addChildNode(subMeshDescriptor);

        if (BitCompare(subMeshDescriptor._componentMask, ComponentType::RIGID_BODY)) {
            subSGN->get<RigidBodyComponent>()->physicsGroup(sgn.get<RigidBodyComponent>()->physicsGroup());
        }

        RenderingComponent* rComp = sgn.get<RenderingComponent>();
        if (rComp != nullptr) {
            RenderingComponent* subRComp = subSGN->get<RenderingComponent>();
            for (auto it : rComp->getShaderBuffers()) {
                subRComp->addShaderBuffer(it);
            }
        }
        
    }

    Object3D::postLoad(sgn);
}

};